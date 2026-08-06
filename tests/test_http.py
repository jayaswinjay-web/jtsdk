#!/usr/bin/env python3
"""Real-world HTTP server tests for JTS GO http_route.

Simulates real browser/client behavior: query strings, POST bodies,
HEAD requests, concurrent connections, and edge-case paths.
"""
import http.client
import socket
import subprocess
import sys
import threading
import time

SERVER = "localhost"
PORT = 9876
PASS = 0
FAIL = 0
FAILURES = []


def check(name, got, expected):
    global PASS, FAIL
    ok = got == expected
    if ok:
        PASS += 1
        print(f"  PASS  {name}")
    else:
        FAIL += 1
        FAILURES.append(name)
        print(f"  FAIL  {name}")
        print(f"        expected: {expected!r}")
        print(f"        got:      {got!r}")


def get(path, headers=None):
    conn = http.client.HTTPConnection(SERVER, PORT, timeout=5)
    conn.request("GET", path, headers=headers or {})
    r = conn.getresponse()
    body = r.read().decode("utf-8", "replace")
    return r.status, dict(r.getheaders()), body


def request(method, path, body=None, headers=None):
    conn = http.client.HTTPConnection(SERVER, PORT, timeout=5)
    conn.request(method, path, body=body, headers=headers or {})
    r = conn.getresponse()
    resp_body = r.read().decode("utf-8", "replace")
    return r.status, dict(r.getheaders()), resp_body


def main():
    print("=" * 60)
    print("JTS GO http_route real-world tests")
    print("=" * 60)

    # ---- Basic page routing ----
    print("\n[1] Basic page routing")
    s, h, b = get("/")
    check("GET / returns 200", s, 200)
    check("GET / body is Home page", b.strip(), "<h1>Home</h1>")

    s, h, b = get("/about")
    check("GET /about returns 200", s, 200)
    check("GET /about body", b.strip(), "<h1>About</h1>")

    s, h, b = get("/contact")
    check("GET /contact returns 200", s, 200)

    # ---- JSON API ----
    print("\n[2] JSON API")
    s, h, b = get("/api/users")
    check("GET /api/users returns 200", s, 200)
    check("JSON content-type detected", h.get("Content-Type", "").startswith("application/json"), True)
    check("JSON body parsed", '"name": "Alice"' in b, True)

    # ---- 404 handling ----
    print("\n[3] 404 handling")
    s, h, b = get("/does-not-exist")
    check("Unknown path returns 404", s, 404)
    s, h, b = get("/")
    check("Known path still works after 404", s, 200)

    # ---- Query strings (real browsers send these) ----
    print("\n[4] Query strings")
    s, h, b = get("/api/user?id=42")
    check("Query string path matches route", s, 200)
    check("Query string served route body", "default" in b, True)

    s, h, b = get("/products?page=2&sort=price")
    check("Multi-param query string", s, 200)
    check("Products body served", "Products" in b, True)

    # ---- POST requests ----
    print("\n[5] POST requests")
    s, h, b = request("POST", "/api/users", body='{"name": "New User"}',
                      headers={"Content-Type": "application/json"})
    check("POST to unregistered route returns 404", s, 404)
    s, h, b = get("/api/users")
    check("GET /api/users unaffected by POST", s, 200)

    # ---- HEAD requests (health checks use these) ----
    print("\n[6] HEAD requests")
    s, h, b = request("HEAD", "/")
    check("HEAD / returns 200", s, 200)
    check("HEAD returns no body", len(b), 0)

    # ---- Case-insensitive method ----
    print("\n[7] Method case-insensitivity")
    s, h, b = request("get", "/about")
    check("lowercase 'get' method matches", s, 200)

    # ---- Concurrency: 20 simultaneous requests ----
    print("\n[8] Concurrency (20 parallel requests)")
    results = []
    def worker(i):
        try:
            s, h, b = get("/api/status")
            results.append((i, s, b))
        except Exception as e:
            results.append((i, -1, str(e)))
    threads = [threading.Thread(target=worker, args=(i,)) for i in range(20)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    ok = all(s == 200 for _, s, _ in results)
    check("All 20 concurrent requests succeeded", ok, True)
    check("Concurrent responses correct", all('"status": "ok"' in b for _, _, b in results), True)

    # ---- Many sequential requests (server stability) ----
    print("\n[9] Stability (100 sequential requests)")
    ok = True
    for i in range(100):
        try:
            s, h, b = get("/")
            if s != 200:
                ok = False
                break
        except Exception:
            ok = False
            break
    check("100 sequential requests OK", ok, True)

    # ---- Large response ----
    print("\n[10] Large response handling")
    conn = http.client.HTTPConnection(SERVER, PORT, timeout=5)
    conn.request("GET", "/api/users")
    r = conn.getresponse()
    body = r.read()
    check("Large-ish response received fully", b'"name": "Bob"' in body.decode(), True)

    print("\n" + "=" * 60)
    print(f"RESULTS: {PASS} passed, {FAIL} failed")
    if FAILURES:
        print("Failed:", FAILURES)
    print("=" * 60)
    return 0 if FAIL == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
