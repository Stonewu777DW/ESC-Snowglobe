import contextlib
import importlib.util
import io
from pathlib import Path
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]

class SetupDryRunTests(unittest.TestCase):
    def test_dry_run_requires_neither_mac_wifi_nor_keychain(self):
        spec = importlib.util.spec_from_file_location('legacy_setup', ROOT / 'tools/setup.py')
        assert spec is not None and spec.loader is not None
        module = importlib.util.module_from_spec(spec)
        with patch('sys.argv', ['setup.py', '--dry-run']):
            spec.loader.exec_module(module)
        output = io.StringIO()
        before = module.CONFIG.read_bytes()
        with patch.object(module, 'detect_ssid', side_effect=AssertionError('Wi-Fi queried')), \
             patch.object(module, 'detect_password', side_effect=AssertionError('Keychain queried')), \
             patch.object(module.subprocess, 'run', side_effect=AssertionError('Process executed')), \
             contextlib.redirect_stdout(output):
            module.main()
        self.assertEqual(module.CONFIG.read_bytes(), before)
        for slug, _ in module.SONGS:
            self.assertIn(f'would write “{slug}”', output.getvalue())
        self.assertIn('would save both to config.h', output.getvalue())
        self.assertIn('would run: arduino-cli upload', output.getvalue())

if __name__ == '__main__':
    unittest.main()
