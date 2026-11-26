#!/usr/bin/env python3
"""BOM 服务联调脚本。

把 design_block 的 base64 数据作为 POST 原始请求体发给本地 BOM 服务，
再把响应里的 base64 解码成 JSON 保存到文件。
"""

import argparse
import base64
import json
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

DEFAULT_URL = "http://127.0.0.1:8080/get_bom_with_all_component_attrs"


def main() -> int:
    parser = argparse.ArgumentParser(description="POST base64 design block data and decode BOM response")
    parser.add_argument("-u", "--url", default=DEFAULT_URL, help="接口地址")
    parser.add_argument("-i", "--input", default="design_block.json.base64", help="请求体文件（base64 或 JSON）")
    parser.add_argument("-o", "--output", default="bom_response.json", help="解码后的 BOM JSON 保存路径")
    parser.add_argument("--raw-output", default="bom_response.json.base64", help="服务响应原文保存路径")
    parser.add_argument("-t", "--timeout", type=float, default=300.0, help="请求超时时间（秒）")
    parser.add_argument("--content-type", default="text/plain; charset=utf-8", help="请求 Content-Type")
    args = parser.parse_args()

    payload = Path(args.input).read_text(encoding="utf-8").strip()

    if payload.startswith(("{", "[")):
        payload = base64.b64encode(payload.encode("utf-8")).decode("ascii")
        print(f"[i] 输入为 JSON，已自动转为 base64（{len(payload)} 字符）")

    request = urllib.request.Request(
        args.url,
        data=payload.encode("utf-8"),
        method="POST",
        headers={"Content-Type": args.content_type},
    )

    started = time.time()
    try:
        with urllib.request.urlopen(request, timeout=args.timeout) as response:
            status = response.status
            content_type = response.headers.get("Content-Type", "")
            raw_bytes = response.read()
    except urllib.error.HTTPError as err:
        body = err.read()[:2000].decode("utf-8", "replace")
        print(f"[x] HTTP {err.code} {err.reason}\n{body}", file=sys.stderr)
        return 1
    except urllib.error.URLError as err:
        print(f"[x] 无法连接 {args.url}: {err.reason}", file=sys.stderr)
        return 1

    elapsed = time.time() - started
    response_text = raw_bytes.decode("utf-8", "replace").strip()
    Path(args.raw_output).write_text(response_text, encoding="utf-8")

    print(f"[+] HTTP {status}, Content-Type: {content_type or '-'}, "
          f"请求体 {len(payload)} 字节, 响应 {len(response_text)} 字节, 耗时 {elapsed:.2f}s")
    print(f"[+] 响应原文已保存: {args.raw_output}")

    try:
        decoded_text = base64.b64decode(response_text, validate=False).decode("utf-8")
    except Exception as err:
        print(f"[x] 响应不是有效的 base64: {err}", file=sys.stderr)
        return 2

    try:
        bom = json.loads(decoded_text)
    except json.JSONDecodeError as err:
        Path(args.output).write_text(decoded_text, encoding="utf-8")
        print(f"[!] 解码结果不是 JSON（{err}），已原样保存: {args.output}", file=sys.stderr)
        return 3

    Path(args.output).write_text(json.dumps(bom, ensure_ascii=False, indent=2), encoding="utf-8")

    rows = bom.get("rows", []) if isinstance(bom, dict) else []
    print(f"[+] BOM JSON 已保存: {args.output}")
    print(f"[+] 顶层字段: {', '.join(bom.keys()) if isinstance(bom, dict) else type(bom).__name__}; 元件行数: {len(rows)}")

    if rows:
        print("[+] 列: " + ", ".join(rows[0].keys()))
        for row in rows[:5]:
            print("    {Reference:<10} {Value:<28} {Footprint}".format(
                Reference=str(row.get("Reference", "")),
                Value=str(row.get("Value", "")),
                Footprint=str(row.get("Footprint", "")),
            ))
        if len(rows) > 5:
            print(f"    ... 其余 {len(rows) - 5} 行见 JSON 文件")

    return 0


if __name__ == "__main__":
    sys.exit(main())
