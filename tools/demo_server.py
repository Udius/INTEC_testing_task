import http.server, json

class Handler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length)
        print("--- POST / ---")
        print(json.dumps(json.loads(body), ensure_ascii=False, indent=2))
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b'{"status":"ok"}')

http.server.HTTPServer(("127.0.0.1", 8080), Handler).serve_forever()