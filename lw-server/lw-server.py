#!/usr/bin/python3

# Lightweight server for "stupids"

import json
import redis
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs

db = redis.StrictRedis(host='localhost', db=0)

#db.hmset('tphash_to_idrange', {'abcafsdf': '11 22'})
#print(db.hget('tphash_to_idrange', 'abcafsdf'))


class RequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse(self.path)

        path = parsed.path
        q_params = parse_qs(parsed.query)

        print(self.path)
        if path == '/tphash':
            tphash = q_params['tphash'][0]

            value = db.hget('tphash_to_idrange', tphash)
            if value is None:
                self.send_response(404)
                return

            ints = [int(x) for x in value.split()]
            first_id, id_count = ints

            res = {
                'first_id': first_id,
                'id_count': id_count,
            }

            # Send response status code
            self.send_response(200)

            # Send headers
            self.send_header('Content-type', 'application/json')
            self.end_headers()

            message = json.dumps(res)

            # Write content as utf-8 data
            self.wfile.write(bytes(message, 'utf8'))
        else:
            self.send_response(404)


server_address = ('127.0.0.1', 1235)
httpd = HTTPServer(server_address, RequestHandler)
print('Running HTTP server...')
httpd.serve_forever()
