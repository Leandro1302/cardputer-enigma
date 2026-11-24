"""Transport edge cases without any physical serial device."""
from pathlib import Path
import sys
import time
import pytest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
import serial_client
from serial_client import Client

class FakePort:
    def __init__(self,chunks):self.chunks=list(chunks);self.writes=[]
    @property
    def in_waiting(self):return len(self.chunks[0]) if self.chunks else 0
    def read(self,n):
        if not self.chunks:return b''
        data=self.chunks[0][:n];self.chunks[0]=self.chunks[0][n:]
        if not self.chunks[0]:self.chunks.pop(0)
        return data
    def write(self,data):self.writes.append(data);return len(data)

def client(chunks):
    c=Client.__new__(Client);c.log=None;c.sequence=0;c.pending=bytearray();c.port=FakePort(chunks);return c

def test_fragmented_json_and_unrelated_messages():
    c=client([b'boot message\n{"id":99,"ok":true}\n{"i',b'd":1,',b'"ok":true,"reply":"PONG"}',b'\n'])
    assert c.command('PING')['reply']=='PONG'
    assert len(c.port.writes)==1

def test_coalesced_responses_are_retained():
    c=client([b'{"id":1,"ok":true}\n{"id":2,"ok":true}\n'])
    assert c.receive(time.monotonic()+1,1)['id']==1
    assert c.receive(time.monotonic()+1,2)['id']==2

def test_mutation_is_never_retried_on_timeout():
    c=client([])
    with pytest.raises(TimeoutError):c.command('ENCRYPT',text='A',timeout=.01)
    assert len(c.port.writes)==1

def test_handshake_deadline_applies_to_unexpected_response(monkeypatch):
    class WrongFirmware(FakePort):
        is_open = False

        def open(self):
            self.is_open = True

        def close(self):
            self.is_open = False

    port = WrongFirmware([])
    monkeypatch.setattr(serial_client.serial, 'Serial', lambda **kwargs: port)
    monkeypatch.setattr(serial_client, 'detect_device', lambda timeout: {'port': 'fake'})
    monkeypatch.setattr(Client, 'command', lambda *args, **kwargs: {'ok': True, 'reply': 'WRONG'})
    with pytest.raises(RuntimeError, match='connection deadline'):
        Client(connect_timeout=0.01)
    assert not port.is_open
