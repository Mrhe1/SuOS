import yaml
import os
def sync_config():
    # 使用绝对路径以确保兼容性
    base_dir = os.path.expanduser('~/SuOS')
    yaml_path = os.path.join(base_dir, 'core/config.yaml')
    h_path = os.path.join(base_dir, 'core/SuOS_Config.h')
    
    if not os.path.exists(yaml_path):
        print(f"Error: {yaml_path} not found.")
        return

    # 1. Load YAML data
    with open(yaml_path, 'r', encoding='utf-8') as f:
        config_data = yaml.safe_load(f)

    # 2. Extract Data
    usrs = config_data.get('usrs', [])
    parts = config_data.get('part', [])
    usr_groups = sorted(list(set(u.get('group') for u in usrs if u.get('group'))))
    
    # Extract ACLs
    usr_acls = sorted(list(set(u.get('acl') for u in usrs if u.get('acl'))))
    part_acls = sorted(list(set(p.get('acl') for p in parts if p.get('acl'))))
    
    # 映射 YAML 数据
    usr_ids = {u['name'].upper(): u['id'] for u in usrs}
    usr_paths = {u['name'].upper(): u.get('path', "") for u in usrs}
    part_map = {p['name'].upper(): p['id'] for p in parts}
    # 补充特定逻辑ID
    logic_parts = {"HEARTBEAT": 10013, "USR_CONTROL": 10014, "USR_REPORT": 10015}
    for k, v in logic_parts.items():
        if k not in part_map:
            part_map[k] = v

    # 3. Read the header file
    with open(h_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # 4. Helper to update namespaces line by line
    def update_ns(target_ns, data_map, lines, type_str="uint32_t", is_string=False):
        in_ns = False
        new_lines = []
        for line in lines:
            if f"namespace {target_ns}" in line:
                in_ns = True
            elif in_ns and line.strip() == "}":
                in_ns = False

            if in_ns and "constexpr" in line:
                parts = line.split()
                # 寻找变量名
                var_name = None
                for i, p in enumerate(parts):
                    if p == "constexpr": continue
                    if p in ["uint32_t", "std::string_view"]: continue
                    if p == "=": break
                    var_name = p

                if var_name and var_name.upper() in data_map:
                    prefix = line.split("constexpr")[0]
                    val = data_map[var_name.upper()]
                    if is_string:
                        line = f'{prefix}constexpr std::string_view {var_name} = "{val}";\n'
                    else:
                        line = f'{prefix}constexpr uint32_t {var_name} = {val};\n'
            new_lines.append(line)
        return new_lines

    # 执行更新
    lines = update_ns("Usr", usr_ids, lines)
    lines = update_ns("UsrPath", usr_paths, lines, "std::string_view", True)
    lines = update_ns("Part", part_map, lines)

    # 5. Update other namespaces with simpler preservation logic
    def update_dynamic_ns(ns_name, items, lines, prefix_comment="//"):
        new_lines = []
        in_ns = False
        stored_comments = []
        for line in lines:
            if f"namespace {ns_name}" in line:
                in_ns = True
                new_lines.append(line)
                continue
            
            if in_ns:
                if line.strip() == "}":
                    # Write collected comments and then the dynamic values
                    for c in stored_comments:
                        new_lines.append(c)
                    for item in items:
                        new_lines.append(f'            constexpr std::string_view {item} = "{item}";\n')
                    new_lines.append(line)
                    in_ns = False
                elif "//" in line:
                    stored_comments.append(line)
                continue
            else:
                new_lines.append(line)
        return new_lines

    lines = update_dynamic_ns("usrGroup", usr_groups, lines)
    lines = update_dynamic_ns("usrAcl", usr_acls, lines)
    lines = update_dynamic_ns("partAcl", part_acls, lines)

    # 4. Write back
    with open(h_path, 'w', encoding='utf-8') as f:
        f.writelines(lines)
    
    print(f"Successfully synced {h_path}")

if __name__ == "__main__":
    sync_config()

