#-------------------------------------------------------------------------------
#   nestestlog.py
#   Parses the nestest.log text file (http://www.qmtpro.com/~nes/misc/nestest.log)
#   into a C array.
#-------------------------------------------------------------------------------

Version = 5

import os.path
import yaml
import genutil

#-------------------------------------------------------------------------------
def gen_header(in_log, out_hdr):

    with open(out_hdr, 'w') as f:
        f.write(f'// #version:{Version}#\n')
        f.write('// machine generated, do not edit!\n')
        f.write('#include <stdint.h>\n')
        f.write('typedef struct {\n')
        f.write('    const char* desc;\n')
        f.write('    uint16_t PC;\n')
        f.write('    uint8_t A,X,Y,P,S;\n')
        f.write('} cpu_state;\n');
        f.write('cpu_state state_table[] = {\n')
        lines = []
        with open(in_log, 'r') as fi:
            lines = fi.readlines()
        for line in lines:
            f.write(
                '  {{ "{}", 0x{},0x{},0x{},0x{},0x{},0x{} }},\n'.format(
                    line[16:48],
                    line[:4],
                    line[50:52],
                    line[55:57],
                    line[60:62],
                    line[65:67],
                    line[71:73],
                )
            )
        f.write('};\n')

#-------------------------------------------------------------------------------
def generate(input, out_src, out_hdr):
    if genutil.isDirty(Version, [input], [out_hdr]):
        gen_header(input, out_hdr)

