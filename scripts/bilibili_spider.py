#!/usr/bin/env python3
"""
B站视频合集爬虫 - 多种实现方式

方式1: 使用Playwright (需要安装)
方式2: 使用Selenium (需要安装)
方式3: 手动输入 (备用方案)

推荐使用方式1，最快最稳定
"""

import json
import re
import sys
from datetime import datetime
from pathlib import Path
import urllib.request
import urllib.parse


# ============================================================
# 方式1: Playwright (推荐)
# ============================================================
def fetch_with_playwright(url: str) -> str:
    """使用Playwright获取页面HTML"""
    try:
        from playwright.sync_api import sync_playwright
    except ImportError:
        return None

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page()
        page.goto(url, wait_until="networkidle")
        content = page.content()
        browser.close()
        return content


# ============================================================
# 方式2: Selenium
# ============================================================
def fetch_with_selenium(url: str) -> str:
    """使用Selenium获取页面HTML"""
    try:
        from selenium import webdriver
        from selenium.webdriver.chrome.options import Options
    except ImportError:
        return None

    options = Options()
    options.add_argument("--headless")
    options.add_argument("--no-sandbox")
    options.add_argument("--disable-dev-shm-usage")

    driver = webdriver.Chrome(options=options)
    driver.get(url)
    html = driver.page_source
    driver.quit()
    return html


# ============================================================
# 方式3: 直接API (带Cookie)
# ============================================================
def fetch_with_api(mid: int, series_id: int, cookie: str = "") -> list:
    """使用B站API获取视频列表"""
    headers = {
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        "Referer": "https://www.bilibili.com"
    }
    if cookie:
        headers["Cookie"] = cookie

    # 尝试多个API endpoint
    apis = [
        f"https://api.bilibili.com/x/series/series_archives_list?mid={mid}&series_id={series_id}&ps=50",
        f"https://api.bilibili.com/x/polymer/web-space/seasons_archives_list?mid={mid}&season_id={series_id}&sort_reverse=false&page_num=1&page_size=50",
    ]

    for api_url in apis:
        try:
            req = urllib.request.Request(api_url, headers=headers)
            with urllib.request.urlopen(req, timeout=10) as resp:
                data = json.loads(resp.read().decode("utf-8"))
                if data.get("code") == 0:
                    archives = data.get("data", {}).get("archives") or data.get("data", {}).get("items", [])
                    if archives:
                        return archives
        except Exception as e:
            continue

    return None


# ============================================================
# HTML解析
# ============================================================
def parse_video_list_from_html(html: str) -> list:
    """从HTML中解析视频列表"""
    videos = []

    # 尝试解析 <script> 标签中的数据
    script_pattern = r'window\.__INITIAL_STATE__\s*=\s*({.*?});'
    match = re.search(script_pattern, html)
    if match:
        try:
            data = json.loads(match.group(1))
            # 根据B站页面结构提取视频数据
            sections = data.get("sectionList", {}).get("sections", [])
            for section in sections:
                for item in section.get("archives", []):
                    videos.append({
                        "bvid": item.get("bvid"),
                        "title": item.get("title"),
                        "desc": item.get("desc"),
                        "pubdate": item.get("pubdate", 0),
                        "length": item.get("length"),
                        "stat": item.get("stat", {})
                    })
        except:
            pass

    return videos


# ============================================================
# 主逻辑
# ============================================================
def format_video_row(project: str, title: str, bvid: str) -> str:
    """格式化视频表格行"""
    url = f"https://www.bilibili.com/video/{bvid}/"
    title_clean = title.replace("|", "\\|").replace("\n", " ")[:50]
    if len(title) > 50:
        title_clean += "..."
    return f"| {project} | {title_clean} | [📺]({url}) | ✅ |"


def generate_video_list_md(videos: list, project_mapping: dict) -> str:
    """生成video_list.md内容"""
    lines = [
        "# 📺 视频列表\n",
        "> 本项目配套视频合集：[现代C++工程实践](https://space.bilibili.com/294645890/lists/7045956)\n",
        "> 最后更新：" + datetime.now().strftime("%Y-%m-%d %H:%M") + "\n",
        "---\n",
        "## 🎬 视频列表\n",
        "| 项目 | 视频标题 | B站链接 | 状态 |",
        "|------|----------|---------|------|",
    ]

    for video in videos:
        title = video.get("title", "")
        bvid = video.get("bvid", "")

        # 匹配项目
        project = "-"
        for keyword, proj_name in project_mapping.items():
            if keyword.lower() in title.lower():
                project = proj_name
                break

        lines.append(format_video_row(project, title, bvid))

    lines.append(f"\n---\n**统计**: 共 {len(videos)} 个视频\n")
    return "\n".join(lines)


def main():
    URL = "https://space.bilibili.com/294645890/lists/7045956"
    MID = 294645890
    SERIES_ID = 7045956

    PROJECT_MAPPING = {
        "argparser": "[ArgParser](./src/ArgParser/)",
        "arg": "[ArgParser](./src/ArgParser/)",
        "命令行": "[ArgParser](./src/ArgParser/)",
        "参数解析": "[ArgParser](./src/ArgParser/)",
        "内存池": "[memory_pool](./project/memory_pool/)",
        "mempool": "[memory_pool](./project/memory_pool/)",
        "iniparser": "[IniParser](./project/IniParser/)",
        "ini": "[IniParser](./project/IniParser/)",
        "mimalloc": "[mimalloc](./project/external/mimalloc/)",
    }

    print("🕷️  B站视频合集爬虫\n")
    print("尝试获取视频列表...")

    videos = None

    # 方式1: 尝试API
    print("\n[1/3] 尝试API方式...")
    videos = fetch_with_api(MID, SERIES_ID)
    if videos:
        print(f"✅ API方式成功! 获取到 {len(videos)} 个视频")

    # 方式2: Playwright
    if not videos:
        print("\n[2/3] 尝试Playwright...")
        print("   (如未安装，请运行: pip install playwright && playwright install)")
        html = fetch_with_playwright(URL)
        if html:
            videos = parse_video_list_from_html(html)
            if videos:
                print(f"✅ Playwright成功! 获取到 {len(videos)} 个视频")

    # 方式3: Selenium
    if not videos:
        print("\n[3/3] 尝试Selenium...")
        print("   (如未安装，请运行: pip install selenium)")
        html = fetch_with_selenium(URL)
        if html:
            videos = parse_video_list_from_html(html)
            if videos:
                print(f"✅ Selenium成功! 获取到 {len(videos)} 个视频")

    if not videos:
        print("\n❌ 所有方式都失败了")
        print("\n建议:")
        print("1. 安装 Playwright: pip install playwright && playwright install")
        print("2. 或者手动在 video_list.md 中维护列表")
        return

    # 按发布时间排序
    videos.sort(key=lambda x: x.get("pubdate", 0))

    # 生成文件
    md_content = generate_video_list_md(videos, PROJECT_MAPPING)
    output_path = Path(__file__).parent.parent / "video_list.md"
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(md_content)

    print(f"\n✅ 已更新 {output_path.name}")
if __name__ == "__main__":
    main()
