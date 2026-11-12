import json
import os
from playwright.sync_api import sync_playwright
BASE=os.environ.get('ESC_TEST_BASE_URL', 'http://localhost:8000/').rstrip('/')+'/'
with sync_playwright() as p:
    browser=p.chromium.launch(headless=True)
    page=browser.new_page(viewport={'width':1280,'height':900})
    errors=[]
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.add_init_script('''(() => {
      let wire = '1|E068|ok|-'; let requests=0;
      const char = new EventTarget();
      char.startNotifications = async () => char;
      char.readValue = async () => { char.dispatchEvent(new Event('characteristicvaluechanged')); return new DataView(new TextEncoder().encode(wire).buffer); };
      const device = new EventTarget(); device.name = 'ESC Pedestal E068';
      device.gatt = {connected:false, connect:async () => {device.gatt.connected=true; return {getPrimaryService:async () => ({getCharacteristic:async () => char})}}, disconnect:() => {device.gatt.connected=false;device.dispatchEvent(new Event('gattserverdisconnected'));}};
      Object.defineProperty(navigator,'bluetooth',{value:{requestDevice:async o => {requests++; if(!o.filters[0].services[0]) throw new Error('Missing service filter'); return device;}}});
      window.setWire = s => {wire=s;char.dispatchEvent(new Event('characteristicvaluechanged'));};
      window.pickerRequests = () => requests;
    })()''')
    page.goto(BASE+'wireless.html')
    assert page.locator('#connect').inner_text()=='Connect'
    assert page.locator('#connect').evaluate('(el)=>{let s=getComputedStyle(el);return s.borderRadius==="50%" && el.clientHeight===el.clientWidth}')
    page.click('#connect')
    page.wait_for_function('document.getElementById("transport").textContent === "Bluetooth LE"')
    assert page.evaluate('pickerRequests()')==1
    page.wait_for_timeout(300)
    assert page.locator('#playbackState').inner_text()=='Idle', 'Sound unlock must not show song playback'
    page.evaluate("setWire('1|E068|ok|rudolph')")
    page.wait_for_function('document.getElementById("nowPlaying").textContent.includes("Rudolph")')
    page.wait_for_function('audio.currentTime > 1 && !audio.paused && audio.duration > 0')
    assert page.locator('#playbackState').inner_text()=='Playing'
    assert page.locator('#duration').inner_text()!='—:—'
    page.locator('#volume').fill('0.35')
    assert page.evaluate('audio.volume')==0.35
    assert page.locator('#volumeValue').inner_text()=='35%'
    page.evaluate("setWire('1|E068|ok|-')")
    page.wait_for_function('audio.paused && document.getElementById("playbackState").textContent === "Idle"')
    page.click('#disconnect')
    assert page.locator('#transport').inner_text()=='Offline'
    for width in [1280,700,390,320]:
        page.set_viewport_size({'width':width,'height':850})
        assert page.evaluate('document.documentElement.scrollWidth<=innerWidth'), width
        assert page.locator('#connect').is_visible()
    page.click('a[href="instructions.html"]')
    assert page.locator('h1').inner_text().startswith('A globe.')
    page.click('text=Back to player →')
    # Pages may canonicalize wireless.html to /wireless; verify the player itself.
    assert page.locator('#connect').inner_text()=='Connect'
    assert not errors, errors
    print(json.dumps({'passed':True,'checks':['circular connect','single service-filtered picker request','real ble.js with simulated GATT','read event recursion regression','real MP3 decode and playback','live elapsed/duration','volume','tag removal','disconnect','responsive 320/390/700/1280','instructions navigation'],'page_errors':errors},indent=2))
    browser.close()
