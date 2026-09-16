# Adds the Antenna Bridge entry to builder/navigation.py on the server (idempotent).
import io, sys
p = sys.argv[1]
s = io.open(p, encoding='utf-8').read()
if 'afu.tools/antenna-bridge' in s:
    print('navigation.py: Antenna Bridge already present')
    sys.exit(0)
old = '        ("📉", "Oberwellen messen", "tinySA anstecken: sendet das Gerät sauber?", "https://afu.tools/oberwellen"),\n'
new = old + '        ("📡", "Antenna Bridge", "CAT-Profile für die Antennensteuerung am Funkgerät teilen", "https://afu.tools/antenna-bridge"),\n'
assert old in s, 'Oberwellen entry not found'
io.open(p, 'w', encoding='utf-8', newline='\n').write(s.replace(old, new))
print('navigation.py: Antenna Bridge entry added')
