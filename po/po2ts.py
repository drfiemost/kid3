#!/usr/bin/env python

import re
import glob
import os
import sys


def get_po_translations(fn):
    """
    Read all translations from a .po file, fill them into an associative
    array.
    """
    msgid = b''
    msgstr = b''
    in_msgid = False
    in_msgstr = False
    trans = {}
    msgidre = re.compile(br'^msgid "(.*)"$')
    msgstrre = re.compile(br'^msgstr "(.*)"$')
    strcontre = re.compile(br'^"(.+)"$')
    with open(fn, 'rb') as fh:
        for line in fh:
            line = line.replace(b'\r\n', b'\n')
            m = msgidre.match(line)
            if m:
                if msgid:
                    trans[msgid] = msgstr
                msgid = m.group(1)
                msgstr = b''
                in_msgid = True
                in_msgstr = False
            m = msgstrre.match(line)
            if m:
                msgstr = m.group(1)
                in_msgid = False
                in_msgstr = True
            m = strcontre.match(line)
            if m:
                if in_msgid:
                    msgid += m.group(1)
                elif in_msgstr:
                    msgstr += m.group(1)
    if msgid:
        trans[msgid] = msgstr
    return trans


def set_ts_translations(infn, outfn, trans):
    """
    Set the translations in a .ts file replacing & by &amp;, < by &lt;,
    > by &gt; and ' by &apos;.
    """
    source = b''
    in_source = False
    sourcere = re.compile(br'<source>(.*)</source>')
    sourcebeginre = re.compile(br'<source>(.*)$')
    sourceendre = re.compile(br'^(.*)</source>')
    with open(infn, 'rb') as infh:
        with open(outfn, 'wb') as outfh:
            for line in infh:
                line = line.replace(b'\r\n', b'\n')
                m = sourcere.search(line)
                if m:
                    source = m.group(1)
                    in_source = False
                else:
                    m = sourcebeginre.search(line)
                    if m:
                        source = m.group(1)
                        in_source = True
                    elif in_source:
                        source += b'\n'
                        m = sourceendre.match(line)
                        if m:
                            source += m.group(1)
                            in_source = False
                        else:
                            source += line.strip()
                    elif b'<translation' in line:
                        source = source \
                            .replace(b'&amp;', b'&') \
                            .replace(b'&lt;', b'<') \
                            .replace(b'&gt;', b'>') \
                            .replace(b'&apos;', b"'") \
                            .replace(b'&quot;', br'\"') \
                            .replace(b'\n', br'\n')
                        if source in trans:
                            translation = trans[source] \
                                .replace(b'&', b'&amp;') \
                                .replace(b'<', b'&lt;') \
                                .replace(b'>', b'&gt;') \
                                .replace(b"'", b'&apos;') \
                                .replace(br'\"', b'&quot;') \
                                .replace(br'\n', b'\n')
                            line = line \
                                .replace(b' type="unfinished"', b'') \
                                .replace(b'</translation>',
                                         translation + b'</translation>')
                        else:
                            print('Could not find translation for "%s"' %
                                  source)
                outfh.write(line)


def generate_ts(lupdate_cmd, podir, srcdir):
    """
    Generate .ts files from .po files.
    parameters: path to lupdate command, directory with po-files,
    directory with source files
    """
    pofiles = glob.glob(os.path.join(podir, '*.po'))
    pofnre = re.compile(r'^.*[\\/]([\w@]+)\.po$')
    languages = [pofnre.sub(r'\1', f) for f in pofiles]
    curdir = os.getcwd()
    sources = []
    for root, dirs, files in os.walk(srcdir):
        for fn in files:
            if fn.endswith('.cpp'):
                sources.append(os.path.join(root, fn))
    os.chdir(srcdir)
    os.system(lupdate_cmd + ' -recursive . -ts ' +
              ' '.join([os.path.join(curdir, 'tmp_' + l + '.ts')
                        for l in languages]))
    os.chdir(curdir)
    for lang in languages:
        tmptsfn = 'tmp_' + lang + '.ts'
        set_ts_translations(tmptsfn, 'kid3_' + lang + '.ts',
                            get_po_translations(
                                os.path.join(podir, lang + '.po')))
        os.remove(tmptsfn)


if __name__ == '__main__':
    generate_ts(*sys.argv[1:4])
