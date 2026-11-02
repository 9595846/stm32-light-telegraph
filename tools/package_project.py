"""Package this project using the same root/firmware/docs layout as the reference ZIP."""
from pathlib import Path, PurePosixPath
import hashlib
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT.parent / 'deliverables'
PREFIX = 'LightTelegraph_Stage2_V1.0'

def include(p):
    rel = p.relative_to(ROOT)
    if '__pycache__' in rel.parts or p.suffix.lower() in ('.pyc', '.bak'):
        return False
    if rel.parts[0] != 'firmware':
        return True
    if len(rel.parts) < 2:
        return True
    folder = rel.parts[1]
    if folder == 'Objects':
        return rel.as_posix() == 'firmware/Objects/Light_Telegraph/Light_Telegraph.axf'
    if folder == 'Listings':
        return False
    if folder == 'BuildLogs':
        return p.suffix == '.log'
    if folder == 'Previews':
        return p.suffix == '.png'
    return not p.name.endswith(('.uvguix', '.uvgui'))

def main():
    fw = ROOT / 'firmware'
    tree = ET.parse(fw / 'Project.uvprojx')
    assert tree.findtext('.//TargetName') == 'Light_Telegraph'
    for name in ('Project.uvprojx', 'Project.uvoptx'):
        assert not (fw / name).read_bytes().startswith(b'\xef\xbb\xbf')
    assert '0 Error(s), 0 Warning(s)' in (fw/'BuildLogs/Keil_Light_Telegraph.log').read_text()
    files = sorted(p for p in ROOT.rglob('*') if p.is_file() and include(p))
    contents = {p.relative_to(ROOT).as_posix(): p.read_bytes() for p in files}
    for element in tree.findall('.//FilePath'):
        rel = PurePosixPath('firmware') / element.text.replace('\\', '/')
        assert str(rel) in contents, f'Missing project dependency: {rel}'
    for rel in ('firmware/release/Light_Telegraph.hex',
                'firmware/Objects/Light_Telegraph/Light_Telegraph.axf',
                'docs/接线图.png', 'docs/03_烧录与验收.md'):
        assert rel in contents, rel
    checksums = ''.join(f'{hashlib.sha256(data).hexdigest()}  {rel}\n' for rel,data in contents.items())
    contents['SHA256SUMS.txt'] = checksums.encode('utf-8')
    DEST.mkdir(exist_ok=True)
    output = DEST / '光影电报机_完整工程_V1.0_20261102.zip'
    suffix = 2
    while output.exists():
        output = DEST / f'光影电报机_完整工程_V1.0_20261102_{suffix}.zip'
        suffix += 1
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for rel,data in contents.items():
            archive.writestr(f'{PREFIX}/{rel}', data)
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None
        for rel,data in contents.items():
            assert archive.read(f'{PREFIX}/{rel}') == data, rel
    print(f'ZIP: {output}')
    print(f'Files: {len(contents)}, bytes: {output.stat().st_size}')
    print('PASS: ZIP CRC, all packaged file bytes, project references, BOM and required artifacts.')

if __name__ == '__main__':
    main()
