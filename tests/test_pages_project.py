import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class PagesProjectTests(unittest.TestCase):
    def setUp(self):
        spec = importlib.util.spec_from_file_location('pages_project', ROOT / 'tools/ensure_pages_project.py')
        assert spec is not None and spec.loader is not None
        self.m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.m)

    def test_existing_main_project_is_not_mutated(self):
        calls = []
        def api(method, path, payload=None):
            calls.append((method, path, payload))
            return {'name': 'esc-snowglobe', 'production_branch': 'main'}
        self.m.ensure_project('a' * 32, api)
        self.assertEqual(len(calls), 1)
        self.assertEqual(calls[0][0], 'GET')

    def test_missing_project_is_created_with_explicit_production_branch(self):
        calls = []
        def api(method, path, payload=None):
            calls.append((method, path, payload))
            if method == 'GET':
                raise self.m.NotFound()
            return {'name': 'esc-snowglobe', 'production_branch': 'main'}
        self.m.ensure_project('a' * 32, api)
        self.assertEqual(calls[-1][2], {'name': 'esc-snowglobe', 'production_branch': 'main'})

    def test_foreign_branch_refuses_without_mutating_project(self):
        with self.assertRaisesRegex(ValueError, 'production branch'):
            self.m.ensure_project('a' * 32, lambda *args: {'name': 'esc-snowglobe', 'production_branch': 'other'})

    def test_invalid_account_is_rejected_before_any_request(self):
        with self.assertRaisesRegex(ValueError, 'account ID'):
            self.m.ensure_project('../wrong', lambda *args: self.fail('unsafe request'))

if __name__ == '__main__':
    unittest.main()
