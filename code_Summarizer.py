import os
import json
import hashlib
import subprocess
from concurrent.futures import ThreadPoolExecutor
from openai import OpenAI

# --- 1. 核心配置 ---
API_KEY = "sk-c5yey1ph8syzcb78t8ph31b8t7wncnxni6ax8109e1g57zgj"
BASE_URL = "https://api.xiaomimimo.com/v1" 
MODEL = "mimo-v2-flash"

client = OpenAI(api_key=API_KEY, base_url=BASE_URL)
DB_FILE = "codebase_summary.json"
TXT_FILE = "codebase_summary.txt"

# 增加 .fbs (FlatBuffers) 和 .py (Python)
ALLOWED_EXTENSIONS = {'.h', '.hpp', '.fbs', '.py'}

def get_all_git_tracked_files():
    """获取 Git 跟踪的所有文件，自动排除 .gitignore 里的内容"""
    try:
        # 获取所有已跟踪的文件
        result = subprocess.check_output(
            ["git", "ls-files"], 
            text=True
        )
        files = result.splitlines()
        # 过滤后缀
        return [f for f in files if os.path.splitext(f)[1].lower() in ALLOWED_EXTENSIONS]
    except Exception as e:
        print(f"Git 读取失败: {e}")
        return []

def get_hash(content):
    """计算文件内容的唯一指纹"""
    return hashlib.md5(content.encode('utf-8', errors='ignore')).hexdigest()

def ask_ai(path, content):
    """强制 JSON Mode 返回总结"""
    print(f"  [AI 总结中...]: {path}")
    
    # 针对不同后缀给 AI 一点微调提示
    ext = os.path.splitext(path)[1].lower()
    file_type = "C++头文件" if ext in ['.h', '.hpp'] else ("FlatBuffers 定义" if ext == '.fbs' else "Python 脚本")
    
    prompt = f"你是一个资深工程师。请用一句话总结这个 {file_type} 的核心功能（30字以内）。\n路径: {path}\n内容:\n{content[:3500]}"
    
    try:
        res = client.chat.completions.create(
            model=MODEL,
            messages=[
                {"role": "system", "content": "你是一个只输出 JSON 的机器人。格式：{\"s\": \"总结内容\"}"},
                {"role": "user", "content": prompt}
            ],
            response_format={"type": "json_object"}
        )
        return json.loads(res.choices[0].message.content).get("s", "无总结")
    except Exception:
        return "AI 总结失败"

def main():
    # 1. 加载现有的索引数据库
    db = {}
    if os.path.exists(DB_FILE):
        with open(DB_FILE, 'r', encoding='utf-8') as f:
            try:
                db = json.load(f)
            except: db = {}

    # 2. 获取所有文件（由 Git 决定哪些文件不被忽略）
    all_files = get_all_git_tracked_files()
    
    # 记录当前运行中还在的文件，用于清理已删除的文件
    current_scan_paths = set(all_files)

    # 准备需要处理的任务列表
    tasks = []
    for path in all_files:
        if not os.path.exists(path): continue
        with open(path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        h = get_hash(content)
        
        # 核心逻辑：如果路径不在索引里，或者哈希值变了，才请求 AI
        if path not in db or db[path].get("h") != h:
            tasks.append((path, content, h))

    # 并行执行 AI 总结
    updated_count = 0
    if tasks:
        print(f"开始并行扫描 {len(tasks)} 个变更文件 (并发 20)...")
        with ThreadPoolExecutor(max_workers=20) as executor:
            # 使用列表推导式分发任务
            results = list(executor.map(lambda t: (t[0], ask_ai(t[0], t[1]), t[2]), tasks))

            for path, summary, h in results:
                db[path] = {"s": summary, "h": h}
        updated_count += 1

        # 保存一次完整的更新
        with open(DB_FILE, 'w', encoding='utf-8') as f:
            json.dump(db, f, ensure_ascii=False, indent=2)

    # 3. 清理工作：如果文件已经在磁盘上删除了，也从索引里删掉
    deleted_files = set(db.keys()) - current_scan_paths
    if deleted_files:
        for df in deleted_files:
            del db[df]
            updated_count += 1
        with open(DB_FILE, 'w', encoding='utf-8') as f:
            json.dump(db, f, ensure_ascii=False, indent=2)

    # 4. 生成最终给 Continue 看的文本
    if updated_count > 0 or not os.path.exists(TXT_FILE):
        with open(TXT_FILE, 'w', encoding='utf-8') as f:
            f.write("项目核心文件功能索引 (全量感知):\n\n")
            # 按字母顺序排个序，方便阅读
            for p in sorted(db.keys()):
                f.write(f"[{p}] : {db[p]['s']}\n")
        print(f"\n✅ 同步完成！共处理了 {len(tasks)} 次AI总结任务，清理了 {len(deleted_files)} 个已删除文件。")
    else:
        print("\n✨ 没有任何变化，索引已是最新。")

if __name__ == "__main__":
    main()