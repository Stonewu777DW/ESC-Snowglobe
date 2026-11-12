#!/usr/bin/env python3
"""Check/create only this project's Cloudflare Pages target; never print credentials."""
import json
import os
import re
import urllib.error
import urllib.request

PROJECT = 'esc-snowglobe'

class NotFound(Exception):
    pass

def ensure_project(account, api):
    if not re.fullmatch(r'[a-f0-9]{32}', account):
        raise ValueError('Invalid Cloudflare account ID')
    path = '/accounts/' + account + '/pages/projects'
    try:
        result = api('GET', path + '/' + PROJECT)
    except NotFound:
        result = api('POST', path, {'name': PROJECT, 'production_branch': 'main'})
    if result.get('name') != PROJECT or result.get('production_branch') != 'main':
        raise ValueError('Unexpected project or production branch; refusing to modify it')
    return result

def main():
    token = os.environ['CLOUDFLARE_API_TOKEN']
    account = os.environ['CLOUDFLARE_ACCOUNT_ID']
    def api(method, path, payload=None):
        body = json.dumps(payload).encode() if payload is not None else None
        req = urllib.request.Request('https://api.cloudflare.com/client/v4' + path,
            data=body, method=method, headers={'Authorization': 'Bearer ' + token,
            'Content-Type': 'application/json'})
        try:
            with urllib.request.urlopen(req, timeout=30) as response:
                data = json.load(response)
        except urllib.error.HTTPError as error:
            if error.code == 404:
                raise NotFound() from error
            raise RuntimeError('Cloudflare API returned HTTP ' + str(error.code)) from None
        if not data.get('success'):
            raise RuntimeError('Cloudflare did not confirm the request')
        return data['result']
    result = ensure_project(account, api)
    print(json.dumps({'name': result['name'], 'production_branch': result['production_branch'],
                      'subdomain': result.get('subdomain')}, indent=2))

if __name__ == '__main__':
    main()
