"""Bounded request/response transport; mutating commands are never retried silently."""
import json
import time
import serial
from common import detect_device

class Client:
    def __init__(self, log=None, connect_timeout=25):
        self.log=log
        self.sequence=0
        self.pending=bytearray()
        # Linux cdc_acm asserts both lines at open. Let pyserial keep that pair.
        # Deasserting them individually passes through a USB Serial/JTAG reset state.
        self.port=serial.Serial(port=None,baudrate=115200,timeout=0.1,write_timeout=2,exclusive=True)
        deadline=time.monotonic()+connect_timeout
        last_error=None
        while time.monotonic()<deadline:
            try:
                if not self.port.is_open:
                    self.port.port=detect_device(min(3,max(0,deadline-time.monotonic())))['port']
                    self.port.open()
                    self.pending.clear()
                reply=self.command('PING',timeout=min(1,max(0,deadline-time.monotonic())))
                if reply.get('ok') is True and reply.get('reply')=='PONG':
                    return
                last_error=RuntimeError('Device returned an unexpected PING reply')
            except (TimeoutError,serial.SerialException,RuntimeError) as error:
                last_error=error
            self.port.close()
            time.sleep(min(0.2,max(0,deadline-time.monotonic())))
        raise RuntimeError('Firmware did not answer PING within connection deadline') from last_error
    def record(self,direction,data):
        if self.log:
            self.log.write(json.dumps({'monotonic':time.monotonic(),'direction':direction,'data':data})+'\n')
            self.log.flush()
    def receive(self,deadline,identifier=None):
        while time.monotonic()<deadline:
            if b'\n' not in self.pending:
                self.pending.extend(self.port.read(max(1,min(self.port.in_waiting,4096))))
                if len(self.pending)>32768:
                    self.pending.clear()
                    raise RuntimeError('Serial response exceeded 32 KiB limit')
                if b'\n' not in self.pending:continue
            line,_,tail=self.pending.partition(b'\n')
            self.pending=bytearray(tail)
            try:data=json.loads(line)
            except (ValueError,UnicodeError):
                self.record('non_json',line.decode('utf8','replace')[:512]);continue
            self.record('rx',data)
            if isinstance(data,dict) and (identifier is None or data.get('id')==identifier):return data
        raise TimeoutError('No matching serial response before deadline; command was not resent')
    def command(self,cmd,timeout=4,**fields):
        self.sequence+=1
        data={'id':self.sequence,'cmd':cmd,**fields}
        self.record('tx',data)
        self.port.write((json.dumps(data,separators=(',',':'))+'\n').encode())
        return self.receive(time.monotonic()+timeout,self.sequence)
    def raw(self,data,timeout=4):
        self.record('tx_raw',repr(data[:80]))
        self.port.write(data)
        return self.receive(time.monotonic()+timeout)
    def close(self):self.port.close()
    def __enter__(self):return self
    def __exit__(self,*args):self.close()
