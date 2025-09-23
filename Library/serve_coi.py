from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import mimetypes

mimetypes.add_type('application/wasm', '.wasm')

class COIHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Resource-Policy', 'same-origin')
        super().end_headers()

if __name__ == "__main__":
    ThreadingHTTPServer(("127.0.0.1", 8000), COIHandler).serve_forever()