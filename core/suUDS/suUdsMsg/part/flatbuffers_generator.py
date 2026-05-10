#!/usr/bin/env python3
"""
FlatBuffers Code Generator - Fixed f-string compatibility
"""

import subprocess
import sys
import re
import os
from pathlib import Path
from typing import List, Dict
from dataclasses import dataclass
import argparse

@dataclass
class TableField:
    name: str
    type: str 
    is_vector: bool = False
    is_string: bool = False

@dataclass
class TableInfo:
    name: str
    fields: List[TableField]
    is_empty: bool = False

@dataclass
class UnionInfo:
    name: str
    members: List[str]

@dataclass
class SchemaInfo:
    namespace: str
    tables: Dict[str, TableInfo]
    unions: Dict[str, UnionInfo]
    root_type: str

class FlatBuffersParser:
    def __init__(self, fbs_path: str):
        self.fbs_path = Path(fbs_path)
        self.content = self.fbs_path.read_text(encoding='utf-8')
            
    def parse(self) -> SchemaInfo:
        namespace = self._extract_namespace()
        tables = self._extract_tables()
        unions = self._extract_unions()
        root_type = self._extract_root_type()
        return SchemaInfo(namespace, tables, unions, root_type)
        
    def _extract_namespace(self) -> str:
        match = re.search(r'namespace\s+([\w.]+)\s*;', self.content)
        return match.group(1) if match else ""
        
    def _extract_tables(self) -> Dict[str, TableInfo]:
        tables = {}
        table_pattern = r'table\s+(\w+)\s*\{([^}]*)\}'
        for match in re.finditer(table_pattern, self.content, re.DOTALL):
            table_name, body = match.groups()
            fields = []
            field_pattern = r'(\w+)\s*:\s*([^;]+);'
            for f_match in re.finditer(field_pattern, body):
                f_name, f_type = f_match.groups()
                f_type = f_type.strip()
                is_vector = '[' in f_type
                is_string = (f_type == 'string')
                clean_type = f_type.replace('[', '').replace(']', '').strip()
                fields.append(TableField(
                    name=f_name, 
                    type=clean_type, 
                    is_vector=is_vector, 
                    is_string=is_string
                ))
            tables[table_name] = TableInfo(
                name=table_name,
                fields=fields,
                is_empty=(len(fields) == 0)
            )
        return tables
        
    def _extract_unions(self) -> Dict[str, UnionInfo]:
        unions = {}
        union_pattern = r'union\s+(\w+)\s*\{([^}]+)\}'
        for match in re.finditer(union_pattern, self.content, re.DOTALL):
            u_name, u_body = match.groups()
            members = [m.strip().rstrip(',') for m in u_body.split('\n') if m.strip() and not m.strip().startswith('//')]
            unions[u_name] = UnionInfo(u_name, members)
        return unions
        
    def _extract_root_type(self) -> str:
        match = re.search(r'root_type\s+(\w+)\s*;', self.content)
        return match.group(1) if match else ""

class CodeGenerator:
    def __init__(self, schema: SchemaInfo, output_dir: str, flatc_path: str, fbs_path: str):
        self.schema = schema
        self.output_dir = Path(output_dir)
        self.flatc_path = Path(flatc_path)
        self.fbs_path = Path(fbs_path)
        
    def _get_class_name(self) -> str:
        stem = self.fbs_path.stem
        return stem[5:] if stem.startswith('suUds') else stem

    def _get_cpp_type(self, fb_type: str) -> str:
        mapping = {
            'uint8': 'uint8_t', 'ubyte': 'uint8_t', 'uint16': 'uint16_t', 
            'uint32': 'uint32_t', 'uint64': 'uint64_t', 'int8': 'int8_t', 
            'byte': 'int8_t', 'int16': 'int16_t', 'int32': 'int32_t', 
            'int64': 'int64_t', 'bool': 'bool', 'float': 'float', 
            'double': 'double', 'string': 'std::string'
        }
        return mapping.get(fb_type, fb_type)

    def generate(self):
        self.output_dir.mkdir(parents=True, exist_ok=True)
        subprocess.run([str(self.flatc_path), '--cpp', '-o', str(self.output_dir), str(self.fbs_path)], check=True)
        
        class_name = self._get_class_name()
        ns_cpp = self.schema.namespace.replace('.', '::')
        union_name = list(self.schema.unions.keys())[0]
        members = self.schema.unions[union_name].members

        # 1. Builder
        builder_path = self.output_dir / f"Uds_{class_name}Builder.hpp"
        methods = []
        for m in members:
            table = self.schema.tables.get(m)
            if not table: continue
            
            cpp_args_list = []
            fbb_calls_list = []
            create_args_list = ["fbb_"]
            
            for f in table.fields:
                if f.is_string:
                    cpp_args_list.append(f"const std::string& {f.name}")
                    fbb_calls_list.append(f"            auto {f.name}__ = fbb_.CreateString({f.name});")
                    create_args_list.append(f"{f.name}__")
                elif f.is_vector:
                    cpp_args_list.append(f"const std::vector<{self._get_cpp_type(f.type)}>& {f.name}")
                    fbb_calls_list.append(f"            auto {f.name}__ = fbb_.CreateVector({f.name});")
                    create_args_list.append(f"{f.name}__")
                else:
                    cpp_args_list.append(f"{self._get_cpp_type(f.type)} {f.name}")
                    create_args_list.append(f.name)

            cpp_args_str = ", ".join(cpp_args_list)
            fbb_calls_str = "\n".join(fbb_calls_list)
            create_args_str = ", ".join(create_args_list)

            method = f"""        // 构建 {m} 消息
        LockGuard Build{m}({cpp_args_str}) {{
            if (!_runtime->isInEventLoop()) throw std::runtime_error("Not in event loop");
            fbb_.Clear();
{fbb_calls_str}
            auto table = Create{m}({create_args_str});
            auto root = Create{self.schema.root_type}(fbb_, {union_name}_{m}, table.Union());
            fbb_.Finish(root);
            return LockGuard(fbb_);
        }}
"""
            methods.append(method)

        all_methods = "".join(methods)
        builder_content = f"""#pragma once
#ifndef UDS_{class_name.upper()}_BUILDER_HPP
#define UDS_{class_name.upper()}_BUILDER_HPP
#include "{self.fbs_path.stem}_generated.h"
#include "suRuntime.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
namespace {ns_cpp} {{
    class {class_name}Builder {{
    public:
        class LockGuard {{
        public:
            LockGuard(flatbuffers::FlatBufferBuilder& fbb) : _fbb(fbb) {{}}
            const uint8_t* data() const {{ return _fbb.GetBufferPointer(); }}
            size_t size() const {{ return _fbb.GetSize(); }}
        private:
            flatbuffers::FlatBufferBuilder& _fbb;
        }};
        {class_name}Builder(std::shared_ptr<SuOS::Runtime::suRuntime> runtime) : _runtime(runtime) {{}}
{all_methods}
        flatbuffers::FlatBufferBuilder& fbb() {{ return fbb_; }}
    private:
        flatbuffers::FlatBufferBuilder fbb_{{ 1024 }};
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
    }};
}}
#endif"""
        builder_path.write_text(builder_content, encoding='utf-8')

        # 2. Parser
        parser_path = self.output_dir / f"Uds_{class_name}Parser.hpp"
        cb_defs_list = []
        cb_vars_list = []
        for m in members:
            table = self.schema.tables.get(m)
            if not table: continue
            
            cpp_args_list = []
            for f in table.fields:
                if f.is_string:
                    cpp_args_list.append(f"const std::string& {f.name}")
                elif f.is_vector:
                    cpp_args_list.append(f"const std::vector<{self._get_cpp_type(f.type)}>& {f.name}")
                else:
                    cpp_args_list.append(f"{self._get_cpp_type(f.type)} {f.name}")
            args_str = ", ".join(cpp_args_list)
            
            cb_defs_list.append(f"        using {m}Callback = std::function<void({args_str})>;")
            cb_vars_list.append(f"            {m}Callback on{m};")
        
        cb_defs_str = "\n".join(cb_defs_list)
        cb_vars_str = "\n".join(cb_vars_list)
        
        cases_list = []
        for m in members:
            table = self.schema.tables.get(m)
            if not table: continue
            
            extract_list = []
            call_args_list = []
            for f in table.fields:
                if f.is_string:
                    extract_list.append(f"                    std::string {f.name}_val = table->{f.name}() ? table->{f.name}()->str() : \"\";")
                    call_args_list.append(f"{f.name}_val")
                elif f.is_vector:
                    cpp_t = self._get_cpp_type(f.type)
                    extract_list.append(f"                    std::vector<{cpp_t}> {f.name}_val;")
                    extract_list.append(f"                    if (table->{f.name}()) {f.name}_val.assign(table->{f.name}()->begin(), table->{f.name}()->end());")
                    call_args_list.append(f"{f.name}_val")
                else:
                    extract_list.append(f"                    auto {f.name}_val = table->{f.name}();")
                    call_args_list.append(f"{f.name}_val")
                    
            extract_str = "\n".join(extract_list)
            if extract_str:
                extract_str = extract_str + "\n"
            call_args_str = ", ".join(call_args_list)
            
            cases_list.append(f"""                case {union_name}_{m}: {{
                    if (_callbacks.on{m}) {{
                        auto table = static_cast<const {m}*>(payload);
                        (void)table;
{extract_str}                        _callbacks.on{m}({call_args_str});
                    }}
                    break;
                }}""")
        all_cases = "\n".join(cases_list)

        parser_content = f"""#pragma once
#ifndef UDS_{class_name.upper()}_PARSER_HPP
#define UDS_{class_name.upper()}_PARSER_HPP
#include "{self.fbs_path.stem}_generated.h"
#include "suRuntime.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <string>
namespace {ns_cpp} {{
    class {class_name}Parser {{
    public:
{cb_defs_str}
        struct Callbacks {{
{cb_vars_str}
        }};
        {class_name}Parser(std::shared_ptr<SuOS::Runtime::suRuntime> runtime, Callbacks cbs) : _runtime(runtime), _callbacks(cbs) {{}}
        void Parse(const uint8_t* buffer, size_t size) {{
            if (!_runtime->isInEventLoop()) return;
            flatbuffers::Verifier v(buffer, size);
            if (!Verify{self.schema.root_type}Buffer(v)) return;
            auto env = Get{self.schema.root_type}(buffer);
            auto payload = env->payload();
            switch (env->payload_type()) {{
{all_cases}
                default: break;
            }}
        }}
    private:
        std::shared_ptr<SuOS::Runtime::suRuntime> _runtime;
        Callbacks _callbacks;
    }};
}}
#endif"""
        parser_path.write_text(parser_content, encoding='utf-8')

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("fbs_path")
    args = parser.parse_args()
    
    flatc = os.path.expanduser("~/Lyra-sdk/buildroot/output/rockchip_rk3506_luckfox/host/bin/flatc")
    p = FlatBuffersParser(args.fbs_path)
    info = p.parse()
    CodeGenerator(info, str(Path(args.fbs_path).parent), flatc, args.fbs_path).generate()
    print("Success")