"""Independent differential checks against uv-pinned Py-Enigma, never our own oracle."""
from pathlib import Path
import random
import subprocess
import pytest
from enigma.machine import EnigmaMachine

ROOT = Path(__file__).resolve().parents[2]
NAMES = ["I", "II", "III", "IV", "V", "VI", "VII", "VIII"]

@pytest.mark.parametrize("seed", range(80))
def test_independent_reference(seed):
    rng = random.Random(seed)
    ids = rng.sample(range(1,9),3)
    rings = ''.join(rng.choices('ABCDEFGHIJKLMNOPQRSTUVWXYZ', k=3))
    positions = ''.join(rng.choices('ABCDEFGHIJKLMNOPQRSTUVWXYZ', k=3))
    reflector = rng.choice('BC')
    shuffled = rng.sample(list('ABCDEFGHIJKLMNOPQRSTUVWXYZ'),26)
    plugs = ' '.join(''.join(shuffled[i:i+2]) for i in range(0, rng.randrange(11)*2, 2))
    message = ''.join(rng.choices('ABCDEFGHIJKLMNOPQRSTUVWXYZ', k=800))
    reference = EnigmaMachine.from_key_sheet(rotors=' '.join(NAMES[i-1] for i in ids),
        reflector=reflector, ring_settings=' '.join(rings), plugboard_settings=plugs)
    reference.set_display(positions)
    expected = reference.process_text(message)
    result = subprocess.run([str(ROOT/'.pio/native/enigma_cli'), ''.join(map(str,ids)),
        rings, positions, reflector, plugs, message], check=True, capture_output=True, text=True, timeout=10)
    actual, state = result.stdout.splitlines()
    assert actual == expected
    assert state == reference.get_display()
