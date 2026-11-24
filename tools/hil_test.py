#!/usr/bin/env python3
"""Actual-device regression tests, independent reference checks, and framebuffer captures."""
import argparse
import json
import random
import struct
import time
import traceback
import zlib
from datetime import datetime,timezone
from enigma.machine import EnigmaMachine
from common import ROOT, FLASH_BYTES, configured_serial
from serial_client import Client

DEFAULT=dict(rotors=[1,2,3],rings='AAA',positions='AAA',reflector='B',plugboard='')
NAMES=['I','II','III','IV','V','VI','VII','VIII']

def screenshot(client,path):
    raw=bytearray()
    for y in range(135):
        try:
            r=client.command('SCREEN',row=y)
        except TimeoutError:
            # SCREEN only reads pixels. Retry once with a new request ID and
            # discard any partial response to the timed-out request.
            client.record('screen_retry', {'row': y})
            client.pending.clear()
            r=client.command('SCREEN',row=y)
        assert r['ok'] and r['row']==y
        row=bytes.fromhex(r['rgb565']);assert len(row)==480
        raw.append(0)
        for (pixel,) in struct.iter_unpack('>H',row):
            red=(pixel>>11)&31;green=(pixel>>5)&63;blue=pixel&31
            raw.extend(((red*255)//31,(green*255)//63,(blue*255)//31))
    def chunk(name,data):
        return struct.pack('>I',len(data))+name+data+struct.pack('>I',zlib.crc32(name+data)&0xffffffff)
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',240,135,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')
    path.write_bytes(png)

def run(client,out,checks,stability):
    def check(name,condition):
        checks.append({'name':name,'passed':bool(condition)})
        if not condition:raise AssertionError(name)
    def command(cmd,**kwargs):
        response=client.command(cmd,**kwargs)
        check('command '+cmd,response.get('ok') is True)
        return response
    info=command('INFO');(out/'firmware-info.json').write_text(json.dumps(info,indent=2))
    check('firmware identity',info['firmware']=='enigma-cardputer' and info['version']=='0.1.0' and info['mac']==configured_serial())
    check('chip and physical flash',info['chip']=='ESP32-S3' and info['flash_bytes']==FLASH_BYTES)
    check('detected Cardputer board',info['board_compatible'] and info['board_id'] in (14,24))
    check('display initialized',info['display_ready'] and (info['display_width'],info['display_height'])==(240,135))
    check('landscape mounting rotation',info['display_rotation']==0)
    baseline=command('DIAG')
    check('firmware self-test',command('SELFTEST')['passed'])
    command('CONFIG',**DEFAULT)
    check('published AAAAA vector',command('ENCRYPT',text='AAAAA')['ciphertext']=='BDZGO')
    check('state after five letters',command('STATE')['positions']=='AAF')
    command('RESET');check('decryption symmetry',command('ENCRYPT',text='BDZGO')['ciphertext']=='AAAAA')
    command('RESET');check('uppercase and punctuation',command('ENCRYPT',text='hello world!')['ciphertext']=='ILBDA AMTAZ!')
    command('CONFIG',**{**DEFAULT,'rotors':[3,2,1],'positions':'KDO','rings':'BUL'})
    for state in ['KDP','KDQ','KER','LFS','LFT','LFU']:
        check('published double-step '+state,command('ENCRYPT',text='A')['positions']==state)
    for naval in [6,7,8]:
        for notch in ['M','Z']:
            command('CONFIG',**{**DEFAULT,'rotors':[1,2,naval],'positions':'AA'+notch})
            check(f'naval turnover {naval}/{notch}',command('ENCRYPT',text='A')['positions']=='AB'+chr((ord(notch)-65+1)%26+65))
    command('CONFIG',rotors=[3,6,8],rings='AHM',positions='UZV',reflector='B',plugboard='AN EZ HK IJ LR MQ OT PV SW UX')
    cipher='YKAENZAPMSCHZBFOCUVMRMDPYCOFHADZIZMEFXTHFLOLPZLFGGBOTGOXGRETDWTJIQHLMXVJWKZUASTR'
    plain='STEUEREJTANAFJORDJANSTANDORTQUAAACCCVIERNEUNNEUNZWOFAHRTZWONULSMXXSCHARNHORSTHCO'
    check('historical Scharnhorst message',command('ENCRYPT',text=cipher)['ciphertext']==plain)
    command('CONFIG',rotors=[2,4,5],rings='BUL',positions='WXC',reflector='B',plugboard='AV BS CG DL FU HZ IN KM OW RX')
    check('published message key',command('ENCRYPT',text='KCH')['ciphertext']=='BLA')
    command('CONFIG',**DEFAULT)
    original=command('STATE')
    for field,value in [('rotors',[1,1,3]),('rotors',[1,2,9]),('rotors',[1,2]),('rings','AA'),('positions','12A'),('reflector','B-Thin'),('plugboard','AB AC'),('plugboard','AA')]:
        response=client.command('CONFIG',**{**DEFAULT,field:value})
        check('reject invalid '+field,response.get('ok') is False and bool(response['error']['code']))
        current=command('STATE');check('invalid configuration is atomic',current['config']==original['config'] and current['positions']==original['positions'])
    for text in ['A'*513,'hello\nworld','é','A\x00B']:
        check('reject invalid text',client.command('ENCRYPT',text=text).get('ok') is False)
    check('unknown command',client.command('NONEXISTENT')['error']['code']=='UNKNOWN_COMMAND')
    check('unknown field',client.command('PING',unexpected=True)['error']['code']=='UNKNOWN_FIELD')
    check('malformed JSON',client.raw(b'{invalid}\n')['error']['code']=='BAD_JSON')
    check('oversize frame',client.raw(b'x'*2100+b'\n')['error']['code']=='LINE_TOO_LONG')
    check('partial-frame timeout',client.raw(b'{"id":',timeout=4)['error']['code']=='FRAME_TIMEOUT')
    check('parser recovers',command('PING')['reply']=='PONG')
    # Long messages cross frame boundaries without resetting state.
    for seed in range(24):
        rng=random.Random(seed);ids=rng.sample(range(1,9),3)
        rings=''.join(rng.choices('ABCDEFGHIJKLMNOPQRSTUVWXYZ',k=3));positions=''.join(rng.choices('ABCDEFGHIJKLMNOPQRSTUVWXYZ',k=3))
        reflector=rng.choice('BC');alphabet=rng.sample(list('ABCDEFGHIJKLMNOPQRSTUVWXYZ'),26)
        plugs=' '.join(''.join(alphabet[i:i+2]) for i in range(0,20,2))
        reference=EnigmaMachine.from_key_sheet(rotors=' '.join(NAMES[i-1] for i in ids),ring_settings=' '.join(rings),reflector=reflector,plugboard_settings=plugs)
        reference.set_display(positions)
        command('CONFIG',rotors=ids,rings=rings,positions=positions,reflector=reflector,plugboard=plugs)
        for _ in range(3):
            text=''.join(rng.choices('ABCDEFGHIJKLMNOPQRSTUVWXYZ',k=400))
            response=command('ENCRYPT',text=text)
            check(f'independent oracle {seed}',response['ciphertext']==reference.process_text(text))
            check('independent final positions',response['positions']==reference.get_display())
    # Exercise the real UI model, including rendering, without claiming physical input.
    def key(k):return command('UI_KEY',key=k)['ui']
    startup=command('DIAG')
    check('startup animation completes and renders multiple frames',
          not startup['startup_active'] and startup['startup_frames']>=24)
    key('ESC');ui=command('UI_STATE')['ui']
    for _ in range(ui['selected']):key('LEFT')
    check('carousel wraps left',key('LEFT')['selected']==4)
    screenshot(client,out/'card-help.png')
    check('carousel wraps right',key('RIGHT')['selected']==0)
    screenshot(client,out/'menu.png')
    for index,label in enumerate(['cipher','settings','reset'], start=1):
        check('carousel advances to '+label,key('RIGHT')['selected']==index)
        screenshot(client,out/f'card-{label}.png')
    check('reset card confirms and stays in menu',
          (reset:=key('ENTER'))['screen']=='menu' and reset['notice']=='Message and rotors reset')
    check('carousel shows final card',key('RIGHT')['selected']==4)
    check('carousel advances to first card',key('RIGHT')['selected']==0)
    key('ENTER');check('UI opens configuration',command('UI_STATE')['ui']['screen']=='configure')
    screenshot(client,out/'configuration.png')
    for index,value in enumerate(['321','KDQ','BUL','C','AV BS']):
        key('ENTER');current=command('UI_STATE')['ui']['editor']
        for _ in current:key('BACKSPACE')
        for c in value:key(c)
        key('ENTER');check('UI field accepted',command('UI_STATE')['ui']['screen']=='configure')
        key('DOWN')
    key('ENTER');check('UI applies configuration',command('UI_STATE')['ui']['screen']=='message')
    key('A');u=key('A');check('UI double step',u['positions']=='LFS')
    expected=u['output'];u=key('BACKSPACE');check('UI exact undo',u['positions']=='KER')
    check('UI replay after undo',key('A')['output']==expected)
    key('RESET');check('UI reset state',command('UI_STATE')['ui']['positions']=='KDQ')
    for c in 'ENIGMA M3 ON CARDPUTER':key(c)
    screenshot(client,out/'message.png')
    key('ESC');key('RIGHT');check('UI message card remains usable',key('ENTER')['screen']=='message')
    key('ESC');key('RIGHT');check('UI opens summary',key('ENTER')['screen']=='summary')
    screenshot(client,out/'summary.png')
    key('ESC');key('RIGHT');key('RIGHT');check('UI opens help',key('ENTER')['screen']=='help')
    screenshot(client,out/'help.png');key('ESC')
    # Restore a predictable GUI configuration and leave the app at the main menu.
    key('RIGHT')
    key('ENTER')
    for value in ['123','AAA','AAA','B','']:
        key('ENTER');current=command('UI_STATE')['ui']['editor']
        for _ in current:key('BACKSPACE')
        for c in value:key(c)
        key('ENTER');key('DOWN')
    key('ENTER');key('ESC');command('CONFIG',**DEFAULT)
    before=command('DIAG')
    for i in range(stability):
        check('repeated ping '+str(i),command('PING')['reply']=='PONG')
        if i%10==0:command('STATE')
    after=command('DIAG')
    check('no unexpected reboot',after['uptime_ms']>=baseline['uptime_ms'] and after['requests']>before['requests'])
    check('bounded heap use',after['heap_free']>=before['heap_free']-4096)
    (out/'diagnostics.json').write_text(json.dumps({'initial':baseline,'after_startup':startup,'before_stability':before,'after_stability':after},indent=2))

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--stability-commands',type=int,default=200)
    args=parser.parse_args()
    if not 1<=args.stability_commands<=2000:parser.error('stability-commands must be 1..2000')
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    out=ROOT/'logs'/('hil-'+stamp);out.mkdir(parents=True)
    checks=[];started=time.monotonic();error=None
    try:
        with (out/'serial.jsonl').open('w') as log:
            with Client(log) as client:
                assert client.command('CONFIG', **DEFAULT)['ok']
                advanced = client.command('ENCRYPT', text='A')
                assert advanced['ok'] and advanced['positions']=='AAB'
                uptime_before = client.command('DIAG')['uptime_ms']
            # New serial descriptor and handshake, no assumed /dev/ttyACM numbering.
            with Client(log) as client:
                info=client.command('INFO')
                assert info['ok'] and info['firmware']=='enigma-cardputer'
                checks.append({'name':'serial reconnection','passed':True})
                uptime_after = client.command('DIAG')['uptime_ms']
                state = client.command('STATE')
                assert uptime_after>=uptime_before and state['positions']=='AAB' and state['letters_processed']==1, 'USB reopen reset the device or serial cipher'
                checks.append({'name':'serial reopen preserves uptime and cipher state','passed':True})
                assert client.command('RESET')['ok']
                run(client,out,checks,args.stability_commands)
    except Exception:
        error=traceback.format_exc();print(error)
    report={'passed':error is None,'checks':len(checks),'duration_seconds':time.monotonic()-started,
            'results':checks,'error':error,'manual_display_and_keyboard_validation':'pending'}
    (out/'results.json').write_text(json.dumps(report,indent=2))
    print(json.dumps({'passed':report['passed'],'checks':len(checks),'directory':str(out)},indent=2))
    if error:raise SystemExit(1)

if __name__=='__main__':main()
