#-------------------------------------------------------------------------------
# elftools example: elf_show_debug_sections.py
#
# Show the names of all .debug_* sections in ELF files.
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
from __future__ import print_function
import sys

from elftools.common.py3compat import bytes2str
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


def collect_symbols(filename, objectType):
    collected_symbols = []

    print('In file:', filename)
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)

        for section in elffile.iter_sections():
            if isinstance(section, SymbolTableSection):

                for symbol in section.iter_symbols():
                    #print(bytes2str(symbol.name))
                    #print(symbol['st_size'])
                    #print(symbol.entry['st_info']['type'])
                    if(symbol.entry['st_info']['type'] == objectType):
                        collected_symbols.append([bytes2str(symbol.name), symbol['st_size']])

    return collected_symbols

if __name__ == '__main__':
    for filename in sys.argv[1:]:
        symbols = sorted(collect_symbols(filename, 'STT_OBJECT'), key=lambda tup: tup[1], reverse=True)

        print('Static object memory total:', sum([size for [name, size] in symbols]))

        for symbol in symbols:
            if(symbol[1] > 8):
                print("%40s: %i"%(symbol[0],symbol[1]))

