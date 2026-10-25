"""Small client for the Unreal Editor MCP server (ModelContextProtocol plugin, http://127.0.0.1:8000/mcp).

    python Scripts/ue_mcp.py tools/call '{"name":"list_toolsets","arguments":{}}'
    python Scripts/ue_mcp.py tools/call '{"name":"call_tool","arguments":{...,"arguments":{"script":"@FILE"}}}' script.py

A third argument replaces "@FILE" in the parameters with that file's text (for execute_tool_script).
"""
import json, os, sys, urllib.request, urllib.error
URL = os.environ.get('UE_MCP', 'http://127.0.0.1:8000/mcp')
SID_FILE = os.path.join(os.environ.get('TEMP', '.'), 'ue_mcp_session')

def post(payload, sid=None):
    data = json.dumps(payload).encode()
    req = urllib.request.Request(URL, data=data, method='POST', headers={
        'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream'})
    if sid: req.add_header('Mcp-Session-Id', sid)
    with urllib.request.urlopen(req, timeout=600) as r:
        body = r.read().decode('utf-8', 'replace')
        new_sid = r.headers.get('Mcp-Session-Id')
    if body.lstrip().startswith('event:') or body.lstrip().startswith('data:'):
        body = '\n'.join(l[5:].strip() for l in body.splitlines() if l.startswith('data:'))
    return (json.loads(body) if body.strip() else None), new_sid

def session():
    if os.path.exists(SID_FILE):
        return open(SID_FILE).read().strip()
    res, sid = post({'jsonrpc': '2.0', 'id': 1, 'method': 'initialize', 'params': {
        'protocolVersion': '2025-06-18', 'capabilities': {}, 'clientInfo': {'name': 'claude-code', 'version': '1'}}})
    post({'jsonrpc': '2.0', 'method': 'notifications/initialized'}, sid)
    open(SID_FILE, 'w').write(sid or '')
    return sid

def call(method, params):
    sid = session()
    try:
        res, _ = post({'jsonrpc': '2.0', 'id': 2, 'method': method, 'params': params}, sid)
    except urllib.error.HTTPError as e:
        os.remove(SID_FILE); sid = session()
        res, _ = post({'jsonrpc': '2.0', 'id': 2, 'method': method, 'params': params}, sid)
    if res and 'error' in res and 'session' in json.dumps(res['error']).lower():
        os.remove(SID_FILE); sid = session()
        res, _ = post({'jsonrpc': '2.0', 'id': 2, 'method': method, 'params': params}, sid)
    return res

if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    method = sys.argv[1]
    params = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {}
    if len(sys.argv) > 3:  # script file for execute_tool_script-like calls: replace "@FILE" in params
        code = open(sys.argv[3], encoding='utf-8').read()
        params = json.loads(json.dumps(params).replace('"@FILE"', json.dumps(code)))
    out = call(method, params)
    s = json.dumps(out, ensure_ascii=False, indent=1)
    print(s)
