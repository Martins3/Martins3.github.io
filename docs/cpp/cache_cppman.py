#!/usr/bin/env python3
"""运行 ./cache_cppman.py，把 cppreference 全部缓存到 ~/.cache/cppman。

使用 2025-02-09 英文离线包，缺页从 cppreference.net 补齐。
需要已安装 cppman；兼容当前 Nix 安装。已有缓存会跳过，可重复运行。
"""

import gzip
import importlib
import os
import runpy
import shutil
import sqlite3
import sys
import tarfile
import urllib.parse
import urllib.request
from pathlib import Path

WORK = (
    Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local/share"))
    / "cppreference-offline"
)
ROOT = WORK / "20250209/cppreference-doc-20250209/reference/en.cppreference.com/w"
ARCHIVE_URL = "https://github.com/PeterFeicht/cppreference-doc/releases/download/v20250209/cppreference-doc-20250209.tar.xz"


def load_cppman() -> None:
    # Nix 将依赖路径放在启动脚本里；加载它，但不执行其 main()。
    try:
        importlib.import_module("cppman")
    except ModuleNotFoundError:
        executable = shutil.which("cppman")
        if executable is None:
            sys.exit("请先安装 cppman")
        wrapped = Path(executable).resolve().with_name(".cppman-wrapped")
        interpreter = wrapped.read_text().splitlines()[0].removeprefix("#!")
        if Path(sys.executable).resolve() != Path(interpreter).resolve():
            os.execv(interpreter, [interpreter, *sys.argv])
        runpy.run_path(str(wrapped))


def download(url: str, path: Path) -> None:
    if path.exists():
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    print(f"下载 {url}", flush=True)
    request = urllib.request.Request(url, headers={"User-Agent": "cppman-offline"})
    with urllib.request.urlopen(request, timeout=60) as response:
        data = response.read()
    temporary = path.with_suffix(".tmp")
    temporary.write_bytes(data)
    temporary.replace(path)


def main() -> None:
    load_cppman()
    environ = importlib.import_module("cppman.environ")
    util = importlib.import_module("cppman.util")
    formatter = importlib.import_module("cppman.formatter.cppreference")
    cache = Path(environ.cache_dir) / "cppreference.com"
    cache.mkdir(parents=True, exist_ok=True)
    WORK.mkdir(parents=True, exist_ok=True)

    # 1. 下载并解压英文离线包。
    archive = WORK / "cppreference-doc-20250209.tar.xz"
    download(ARCHIVE_URL, archive)
    marker = WORK / "20250209/.extracted-completely"
    if not marker.exists():
        with tarfile.open(archive) as bundle:
            bundle.extractall(WORK / "20250209", filter="data")
        marker.touch()

    # 2. 在副本上修改索引，全部成功后才替换。保留原有关键词和其他源。
    staged = WORK / "index-building.db"
    shutil.copyfile(environ.index_db, staged)
    backup = WORK / "index-before-simple.db"
    if not backup.exists():
        shutil.copyfile(environ.index_db, backup)
    with sqlite3.connect(staged) as db:
        pages = db.execute('SELECT title,url FROM "cppreference.com"').fetchall()
        for number, (title, url) in enumerate(pages, 1):
            name = title
            if len((name.replace("/", "_") + ".3.gz").encode()) > 255:
                first, *rest = name.split(", ")
                prefix = first.rsplit("::", 1)[0] + "::"
                name = ", ".join([first, *(s.removeprefix(prefix) for s in rest)])
                db.execute(
                    'UPDATE "cppreference.com" SET title=? WHERE url=?', (name, url)
                )
            target = cache / (name.replace("/", "_") + ".3.gz")

            # 3. 找到本地 HTML；缺页从镜像补齐，再用 cppman 转成 man 格式。
            if not target.exists():
                relative = urllib.parse.urlsplit(url).path.removeprefix("/w/") + ".html"
                source = ROOT / relative
                if not source.exists():
                    source = ROOT / urllib.parse.unquote(relative)
                if not source.exists():
                    source = WORK / "supplement-cppreference.net" / relative
                    download("https://cppreference.net/" + relative, source)
                text = formatter.html2groff(util.fixupHTML(source.read_bytes()), title)
                if '.TH "' not in text or '.SH "NAME"' not in text:
                    raise ValueError(f"页面转换失败：{url}")
                temporary = cache / ".page.tmp"
                temporary.write_bytes(gzip.compress(text.encode(), mtime=0))
                temporary.replace(target)

            # 4. 检查每页，避免把损坏的缓存算作成功。
            text = gzip.decompress(target.read_bytes()).decode()
            if '.SH "NAME"' not in text:
                raise ValueError(f"缓存损坏，请删除后重跑：{target}")
            if number % 500 == 0:
                print(f"{number}/{len(pages)}", flush=True)
    local_index = Path(environ.cache_dir) / "index.db"
    temporary_index = local_index.with_suffix(".tmp")
    shutil.copyfile(staged, temporary_index)
    temporary_index.replace(local_index)
    print(f"完成：{len(pages)} 页，缓存目录：{cache}")


if __name__ == "__main__":
    main()
