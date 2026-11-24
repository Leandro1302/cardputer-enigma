#!/usr/bin/env python3
"""One bounded build/package/upload/test cycle. Never blindly reflash on failure."""
import argparse
from datetime import datetime,timezone
import json
from pathlib import Path
import subprocess
import sys
import time
from common import ROOT,local_environment,verified_backup

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--hardware',action='store_true',help='Upload and test the authorized Cardputer after building')
    parser.add_argument('--max-flashes',type=int,default=3,help='Maximum uploads in the named campaign (1..10)')
    parser.add_argument('--campaign',default='development')
    args=parser.parse_args()
    if not 1<=args.max_flashes<=10 or not args.campaign.replace('-','').replace('_','').isalnum():parser.error('Invalid limit or campaign name')
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    out=ROOT/'logs'/('cycle-'+stamp);out.mkdir(parents=True)
    ledger=ROOT/'logs'/('campaign-'+args.campaign+'.json')
    history=json.loads(ledger.read_text()) if ledger.exists() else []
    stages=[('native',['bash','tools/test_native.sh'],180),('build',['bash','tools/build.sh'],600),
            ('package',[sys.executable,'tools/package.py'],60)]
    result={'started_utc':stamp,'hardware_requested':args.hardware,'stages':[],'passed':False}
    try:
        for name,command,timeout in stages:
            with (out/(name+'.log')).open('w') as log:
                run=subprocess.run(command,cwd=ROOT,env=local_environment(),stdout=log,stderr=subprocess.STDOUT,timeout=timeout)
            result['stages'].append({'name':name,'returncode':run.returncode})
            if run.returncode:raise RuntimeError(f'{name} failed; see {out/name}.log')
        if args.hardware:
            verified_backup()
            manifest=json.loads((ROOT/'releases/0.1.0/manifest.json').read_text())
            sha=manifest['merged_sha256']
            if len(history)>=args.max_flashes:raise RuntimeError('Campaign upload limit reached; inspect failures before starting a new campaign')
            if history and history[-1]['sha256']==sha:raise RuntimeError('No firmware change since previous campaign upload; refusing a blind retry')
            # Count attempted uploads as well as successful ones.
            history.append({'utc':stamp,'sha256':sha,'logs':str(out.relative_to(ROOT))})
            ledger.write_text(json.dumps(history,indent=2)+'\n')
            for name,command,timeout in [('flash',[sys.executable,'tools/flash.py'],180),('hil',[sys.executable,'tools/hil_test.py'],240)]:
                with (out/(name+'.log')).open('w') as log:
                    run=subprocess.run(command,cwd=ROOT,env=local_environment(),stdout=log,stderr=subprocess.STDOUT,timeout=timeout)
                result['stages'].append({'name':name,'returncode':run.returncode})
                if run.returncode:raise RuntimeError(f'{name} failed; inspect logs and make a targeted correction')
        result['passed']=True
    except Exception as error:
        result['error']=str(error)
    finally:
        (out/'results.json').write_text(json.dumps(result,indent=2)+'\n')
        print(json.dumps({**result,'logs':str(out)},indent=2))
    if not result['passed']:raise SystemExit(1)

if __name__=='__main__':main()
