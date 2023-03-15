#-------------------------------------------------------------------------------
#   fuse.py
#
#   Convert FUSE test files into C header.
#-------------------------------------------------------------------------------

Version = 2

import os.path
import yaml
import genutil

#-------------------------------------------------------------------------------
def gen_header(desc, out_hdr):
    with open(out_hdr, 'w') as outp:
        outp.write(f'// #version:{Version}#\n')
        outp.write('// machine generated, do not edit!\n')
        for item in desc['files']:
            inp_path = f'{os.path.dirname(out_hdr)}/' + item['file']
            inp_name = item['name']
            outp.write(f'fuse_test_t {inp_name}[] = {{\n')
            with open(inp_path, 'r') as inp:
                line = inp.readline()
                num_tests = 0
                while line:
                    outp.write('  {\n')
                    num_tests += 1
                    # desc (1 byte)
                    val = line.split()[0]
                    outp.write(f'    .desc = "{val}",\n')
                    # optional events start with spaces
                    line = inp.readline()
                    if line[0] == ' ':
                        num_events = 0
                        outp.write('    .events = {\n')
                        while line[0] == ' ':
                            tok = line.split()
                            if len(tok)==4:
                                outp.write('      {{ .tick={}, .type=EVENT_{}, .addr=0x{}, .data=0x{} }},\n'.format(
                                    tok[0], tok[1], tok[2], tok[3]
                                ))
                                num_events += 1
                            line = inp.readline()
                        outp.write('    },\n')
                        outp.write(f'    .num_events = {num_events},\n');
                    # 16-bit registers
                    tok = line.split()
                    outp.write('    .state = {\n')
                    if len(tok) == 12:
                        outp.write(
                            f'      .af=0x{tok[0]}, .bc=0x{tok[1]}, .de=0x{tok[2]}, .hl=0x{tok[3]},\n'
                        )
                        outp.write(
                            f'      .af_=0x{tok[4]}, .bc_=0x{tok[5]}, .de_=0x{tok[6]}, .hl_=0x{tok[7]},\n'
                        )
                        outp.write(
                            f'      .ix=0x{tok[8]}, .iy=0x{tok[9]}, .sp=0x{tok[10]}, .pc=0x{tok[11]},\n'
                        )
                    # additional registers and flags
                    tok = inp.readline().split()
                    if len(tok) == 7:
                        outp.write(
                            f'      .i=0x{tok[0]}, .r=0x{tok[1]}, .iff1={tok[2]}, .iff2={tok[3]}, .im={tok[4]}, .halted={tok[5]}, .ticks={tok[6]}\n'
                        )
                    outp.write('    },\n')
                    # optional memory chunks
                    num_chunks = 0
                    tok = inp.readline().split()
                    if len(tok) > 1:
                        outp.write('    .chunks = {\n')
                        while len(tok) > 0:
                            if tok[0] != '-1':
                                outp.write('      { ')
                                outp.write(f'.addr=0x{tok[0]}, .bytes = {{ ')
                                i = 1
                                while tok[i] != '-1':
                                    outp.write(f'0x{tok[i]},')
                                    i += 1
                                outp.write(f'}}, .num_bytes={i - 1}, ')
                                outp.write('},\n')
                                num_chunks += 1
                            tok = inp.readline().split()
                        outp.write('    },\n')
                    outp.write(f'    .num_chunks = {num_chunks},\n')
                    outp.write('  },\n')
                    line = inp.readline()
            outp.write('};\n')
            outp.write(f'const int {inp_name}_num = {num_tests};\n')

#-------------------------------------------------------------------------------
def generate(input, out_src, out_hdr) :
    if genutil.isDirty(Version, [input], [out_hdr]) :
        with open(input, 'r') as f:
            desc = yaml.load(f)
        gen_header(desc, out_hdr)
