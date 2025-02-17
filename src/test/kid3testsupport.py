import os
import sys
import subprocess


_kid3_cli_path = ''


def kid3_cli_path():
    global _kid3_cli_path
    if not _kid3_cli_path:
        curdir = os.getcwd()
        cli_exe = 'kid3-cli'
        if sys.platform == 'win32':
            cli_exe += '.exe'
        while True:
            cli_path = os.path.join(curdir, 'src', 'app', 'cli', cli_exe)
            if os.path.isfile(cli_path):
                _kid3_cli_path = cli_path
                break
            else:
                cli_path = os.path.join(curdir, cli_exe)
                if os.path.isfile(cli_path):
                    _kid3_cli_path = cli_path
                    break
            parentdir = os.path.dirname(curdir)
            if len(parentdir) < 2 or parentdir == curdir:
                raise FileNotFoundError(cli_exe)
            curdir = parentdir
    return _kid3_cli_path


def call_kid3_cli(args):
    if isinstance(args, str):
        args = [args]
    out = subprocess.check_output([kid3_cli_path()] + args)
    try:
        s = out.decode()
    except UnicodeDecodeError:
        s = out.decode(sys.getfilesystemencoding())
    return s.replace('\r\n', '\n')


def create_test_file(filename):
    ext = os.path.splitext(filename)[1]
    if ext == '.m4a':
        d = b'\x00\x00\x00\x18ftypM4A \x00\x00\x02\x00isomiso2\x00\x00\x00\x08free\x00\x00\x00!mdat\xde\x02\x00' \
            b'Lavc56.41.100\x00\x020@\x0e\x01\x18 \x07\x00\x00\x02\xcdmoov\x00\x00\x00lmvhd\x00\x00\x00\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03\xe8\x00\x00\x00\x18\x00\x01\x00\x00\x01\x00\x00\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
            b'\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00@\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00' \
            b'\x01\xf7trak\x00\x00\x00\\tkhd\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01' \
            b'\x00\x00\x00\x00\x00\x00\x00\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x01\x00\x00\x00' \
            b'\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00@\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00$edts' \
            b'\x00\x00\x00\x1celst\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x04\x00\x00\x01\x00' \
            b'\x00\x00\x00\x01omdia\x00\x00\x00 mdhd\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xacD' \
            b'\x00\x00\x04\x01U\xc4\x00\x00\x00\x00\x00-hdlr\x00\x00\x00\x00\x00\x00\x00\x00soun\x00\x00\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00SoundHandler\x00\x00\x00\x01\x1aminf\x00\x00\x00\x10smhd\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00$dinf\x00\x00\x00\x1cdref\x00\x00\x00\x00\x00\x00\x00\x01\x00' \
            b'\x00\x00\x0curl \x00\x00\x00\x01\x00\x00\x00\xdestbl\x00\x00\x00jstsd\x00\x00\x00\x00\x00\x00\x00' \
            b'\x01\x00\x00\x00Zmp4a\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00' \
            b'\x10\x00\x00\x00\x00\xacD\x00\x00\x00\x00\x006esds\x00\x00\x00\x00\x03\x80\x80\x80%\x00\x01\x00' \
            b'\x04\x80\x80\x80\x17@\x15\x00\x00\x00\x00\x01\xf4\x00\x00\x00!\x9c\x05\x80\x80\x80\x05\x12\x08V' \
            b'\xe5\x00\x06\x80\x80\x80\x01\x02\x00\x00\x00 stts\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x01' \
            b'\x00\x00\x04\x00\x00\x00\x00\x01\x00\x00\x00\x01\x00\x00\x00\x1cstsc\x00\x00\x00\x00\x00\x00\x00' \
            b'\x01\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x01\x00\x00\x00\x1cstsz\x00\x00\x00\x00\x00\x00' \
            b'\x00\x00\x00\x00\x00\x02\x00\x00\x00\x15\x00\x00\x00\x04\x00\x00\x00\x14stco\x00\x00\x00\x00\x00' \
            b'\x00\x00\x01\x00\x00\x00(\x00\x00\x00budta\x00\x00\x00Zmeta\x00\x00\x00\x00\x00\x00\x00!hdlr\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00mdirappl\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08ilst\x00' \
            b'\x00\x00%free\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01' \
            b'\x01\x01\x01\x01\x01\x01\x01\x01'
    elif ext == '.flac':
        d = b'fLaC\x80\x00\x00"\x10\x00\x10\x00\x00\x00\x0c\x00\x00\x0c\n\xc4@\xf0\x00\x00\x00\x01\xc4\x10?\x12-\'g|' \
            b'\x9d\xb1D\xca\xe19Jf\xff\xf8i\x08\x00\x00\x1d\x02\x00\x00 \x0c'
    elif ext == '.spx':
        d = b'OggS\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x0c8_\n\x00\x00\x00\x00\x1a\xb2i\xd1\x01PSpeex   1.2rc1' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00P\x00\x00\x00D\xac\x00\x00\x02' \
            b'\x00\x00\x00\x04\x00\x00\x00\x01\x00\x00\x00\xff\xff\xff\xff\x80\x02\x00\x00\x00\x00\x00\x00\x01\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00OggS\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
            b'\x0c8_\n\x01\x00\x00\x00\xe1j\xaf\xc6\x01!\x19\x00\x00\x00Encoded with Speex 1.2rc1\x00\x00\x00' \
            b'\x00OggS\x00\x04\x01\x00\x00\x00\x00\x00\x00\x00\x0c8_\n\x02\x00\x00\x00b\xc6\xa4\xf9\x01Z>\x9d\x1b' \
            b'\x9a \x00\x01\x7f\xff\xff\xff\xff\xff\xdbm\xb6\xdbm\xb6\x89\x00\xbf\xff\xff\xff\xff\xff\xed\xb6\xdbm' \
            b'\xb6\xdbB\x00_\xff\xff\xff\xff\xff\xf6\xdbm\xb6\xdbm\xa1\x00/\xff\xff\xff\xff\xff\xfbm\xb6\xdbm\xb6' \
            b'\xdb;`\xab\xab\xab\xab\xab\n\xba\xba\xba\xba\xb0\xab\xab\xab\xab\xab\n\xba\xba\xba\xba\xb9;`\x00\x00'
    elif ext == '.mp3':
        d = b'\xff\xfbP\xc4\x00\x03\xc0\x00\x01\xa4\x00\x00\x00 \x00\x004\x80\x00\x00\x04LAME3.99.5UUUUUUUUUUUUUUUU' \
            b'UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU' \
            b'UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU\xff\xfbR\xc4]\x83\xc0\x00\x01\xa4\x00\x00' \
            b'\x00 \x00\x004\x80\x00\x00\x04UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU' \
            b'UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU' \
            b'UUUUUUUUUUUUUUUU'
    elif ext == '.wv':
        d = b'wvpk`\x00\x00\x00\x07\x04\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x05\x18\x80\x04\xfd' \
            b'\xff\xff\xff!\x16RIFF&\x00\x00\x00WAVEfmt \x10\x00\x00\x00\x01\x00\x01\x00D\xac\x00\x00\x88X\x01\x00' \
            b'\x02\x00\x10\x00data\x02\x00\x00\x00\x02\x00\x03\x00\x04\x00\x05\x03\x00\x00\x00\x00\x00\x00e\x02\x00' \
            b'\x00\x00\x00\x8a\x01\x00\x00\xfd\xff'
    elif ext == '.ape':
        d = b'MAC \x96\x0f\x00\x004\x00\x00\x00\x18\x00\x00\x00\x04\x00\x00\x00,\x00\x00\x00\x10\x00\x00\x00\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00@\xd6\x946\xc8\xdd\xe3Q\x83\x89\xf63GuM\xef\xd0\x07\x00\x00\x00 \x01\x00\x01' \
            b'\x00\x00\x00\x01\x00\x00\x00\x10\x00\x01\x00D\xac\x00\x00|\x00\x00\x00RIFF&\x00\x00\x00WAVEfmt \x10' \
            b'\x00\x00\x00\x01\x00\x01\x00D\xac\x00\x00\x88X\x01\x00\x02\x00\x10\x00data\x02\x00\x00\x00\x7f\x89\xec' \
            b'\xa0\x01\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00'
    elif ext == '.wav':
        d = b'RIFFL\x01\x00\x00WAVEfmt \x10\x00\x00\x00\x01\x00\x02\x00D\xac\x00\x00\x10\xb1\x02\x00\x04\x00\x10' \
            b'\x00data(\x01\x00\x00\xd0\xe5l\xe5x\xe5X\xe5\x19\xe5;\xe5\xb6\xe4\x1a\xe5^\xe4\xff\xe4\x06\xe4\xfe' \
            b'\xe4\xbf\xe3\xef\xe4T\xe3\xed\xe4\xf9\xe2\xc7\xe4\x91\xe2\xb2\xe44\xe2\x8a\xe4\xc6\xe1f\xe4d\xe1=' \
            b'\xe4\xf0\xe0\x0b\xe4\x88\xe0\xe0\xe3<\xe0\xb2\xe3\x07\xe0\x8b\xe3\xd4\xdfW\xe3\xb5\xdf2\xe3\xb9\xdf' \
            b'\x06\xe3\xd2\xdf\x03\xe3\xf5\xdf\xf5\xe2!\xe0\xee\xe2R\xe0\xdb\xe2\x96\xe0\xdd\xe2\xc0\xe0\xe7\xe2' \
            b'\xec\xe0\xed\xe2\r\xe1\xea\xe2:\xe1\xf8\xe2X\xe1\x17\xe3s\xe11\xe3\x97\xe1N\xe3\xb3\xe1g\xe3\xd3\xe1' \
            b'\x93\xe3\xd7\xe1\xa6\xe3\xf9\xe1\xad\xe3\x01\xe2\xb4\xe3\x1e\xe2\xb8\xe3&\xe2\xbd\xe3F\xe2\xb5\xe3Z' \
            b'\xe2\xa3\xe3x\xe2\x92\xe3\x97\xe2v\xe3\xb3\xe2w\xe3\xcb\xe2h\xe3\xe8\xe2b\xe3\x0b\xe3O\xe3\x1b\xe3E' \
            b'\xe3-\xe3H\xe3,\xe3C\xe3H\xe3:\xe3;\xe3Q\xe3I\xe3Z\xe3Q\xe3\x84\xe3q\xe3\x9c\xe3\x94\xe3\xdf\xe3\xb5' \
            b'\xe3#\xe4\xe2\xe3f\xe4\x0f\xe4\xb0\xe4O\xe4!\xe5o\xe4\x94\xe5\xa3\xe4\x07\xe6\xbc\xe4X\xe6\xd2\xe4' \
            b'\xc3\xe6\xd6\xe4\x18\xe7\xf1\xe4\x83\xe7\t\xe5\xed\xe7 \xe5_\xe88\xe5\xce\xe8r\xe5C\xe9\xae\xe5\xc9' \
            b'\xe9\xe9\xe5o\xea(\xe6\x06\xeb\x82\xe6\xac\xeb'
    elif ext == '.opus':
        d = b'OggS\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x91d\x87S\x00\x00\x00\x00\xfb\x1f\xdfC\x01\x13OpusHead' \
            b'\x01\x01d\x01D\xac\x00\x00\x00\x00\x00OggS\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x91d\x87S\x01\x00' \
            b'\x00\x002\xa1\x1d5\x01\x1bOpusTags\x0b\x00\x00\x00libopus 1.1\x00\x00\x00\x00OggS\x00\x04f\x01\x00\x00' \
            b'\x00\x00\x00\x00\x91d\x87S\x02\x00\x00\x00\xe6\xe1CE\x01\x03\xf8\xff\xfe'
    elif ext == '.mpc':
        d = b'MPCKSH\x0cp\n\xc9\xab\x08\x01\x00\x1b\x0bRG\x0c\x01\x00\x00\x00\x00\x00\x00\x00\x00EI\x07\xa0\x01\x1e' \
            b'\x01SO\x08\x0c\x00\x00\x00\x00AP\x04\x00ST\x06\x01\x12\xb0SE\x03'
    elif ext == '.aif':
        d = b'FORM\x00\x00\x01VAIFFCOMM\x00\x00\x00\x12\x00\x02\x00\x00\x00J\x00\x10@\x0e\xacD\x00\x00\x00\x00\x00' \
            b'\x00SSND\x00\x00\x010\x00\x00\x00\x00\x00\x00\x00\x00\xe5\xd0\xe5l\xe5x\xe5X\xe5\x19\xe5;\xe4\xb6\xe5' \
            b'\x1a\xe4^\xe4\xff\xe4\x06\xe4\xfe\xe3\xbf\xe4\xef\xe3T\xe4\xed\xe2\xf9\xe4\xc7\xe2\x91\xe4\xb2\xe24' \
            b'\xe4\x8a\xe1\xc6\xe4f\xe1d\xe4=\xe0\xf0\xe4\x0b\xe0\x88\xe3\xe0\xe0<\xe3\xb2\xe0\x07\xe3\x8b\xdf\xd4' \
            b'\xe3W\xdf\xb5\xe32\xdf\xb9\xe3\x06\xdf\xd2\xe3\x03\xdf\xf5\xe2\xf5\xe0!\xe2\xee\xe0R\xe2\xdb\xe0\x96' \
            b'\xe2\xdd\xe0\xc0\xe2\xe7\xe0\xec\xe2\xed\xe1\r\xe2\xea\xe1:\xe2\xf8\xe1X\xe3\x17\xe1s\xe31\xe1\x97' \
            b'\xe3N\xe1\xb3\xe3g\xe1\xd3\xe3\x93\xe1\xd7\xe3\xa6\xe1\xf9\xe3\xad\xe2\x01\xe3\xb4\xe2\x1e\xe3\xb8' \
            b'\xe2&\xe3\xbd\xe2F\xe3\xb5\xe2Z\xe3\xa3\xe2x\xe3\x92\xe2\x97\xe3v\xe2\xb3\xe3w\xe2\xcb\xe3h\xe2\xe8' \
            b'\xe3b\xe3\x0b\xe3O\xe3\x1b\xe3E\xe3-\xe3H\xe3,\xe3C\xe3H\xe3:\xe3;\xe3Q\xe3I\xe3Z\xe3Q\xe3\x84\xe3q' \
            b'\xe3\x9c\xe3\x94\xe3\xdf\xe3\xb5\xe4#\xe3\xe2\xe4f\xe4\x0f\xe4\xb0\xe4O\xe5!\xe4o\xe5\x94\xe4\xa3\xe6' \
            b'\x07\xe4\xbc\xe6X\xe4\xd2\xe6\xc3\xe4\xd6\xe7\x18\xe4\xf1\xe7\x83\xe5\t\xe7\xed\xe5 \xe8_\xe58\xe8' \
            b'\xce\xe5r\xe9C\xe5\xae\xe9\xc9\xe5\xe9\xeao\xe6(\xeb\x06\xe6\x82\xeb\xac'
    elif ext == '.jpg':
        d = b'\xff\xd8\xff\xdb\x00C\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff' \
            b'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff' \
            b'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xdb\x00C\x01\xff' \
            b'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff' \
            b'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff' \
            b'\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xc2\x00\x11\x08\x00\n\x00\n\x03\x01"\x00\x02' \
            b'\x11\x01\x03\x11\x01\xff\xc4\x00\x15\x00\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
            b'\x00\x00\x01\xff\xc4\x00\x15\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01' \
            b'\x02\xff\xda\x00\x0c\x03\x01\x00\x02\x10\x03\x10\x00\x00\x01\x80\xaf\xff\xc4\x00\x14\x10\x01\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00 \xff\xda\x00\x08\x01\x01\x00\x01\x05\x02\x1f\xff' \
            b'\xc4\x00\x14\x11\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xda\x00\x08' \
            b'\x01\x03\x01\x01?\x01\x7f\xff\xc4\x00\x14\x11\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' \
            b'\x00\x00\x00\xff\xda\x00\x08\x01\x02\x01\x01?\x01\x7f\xff\xc4\x00\x14\x10\x01\x00\x00\x00\x00\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00 \xff\xda\x00\x08\x01\x01\x00\x06?\x02\x1f\xff\xc4\x00\x14\x10\x01' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00 \xff\xda\x00\x08\x01\x01\x00\x01?!\x1f' \
            b'\xff\xda\x00\x0c\x03\x01\x00\x02\x00\x03\x00\x00\x00\x10\x0b\xff\xc4\x00\x14\x11\x01\x00\x00\x00\x00' \
            b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xda\x00\x08\x01\x03\x01\x01?\x10\x7f\xff\xc4\x00' \
            b'\x14\x11\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xff\xda\x00\x08\x01\x02' \
            b'\x01\x01?\x10\x7f\xff\xc4\x00\x14\x10\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00 ' \
            b'\xff\xda\x00\x08\x01\x01\x00\x01?\x10\x1f\xff\xd9'
    else:
        d = b''
    with open(filename, 'wb') as fh:
        fh.write(d)
