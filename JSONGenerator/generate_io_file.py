import common
import argparse
import logging

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Exclude unused files from the IOFile generated by Nextflow using the strace output')
    parser.add_argument('workdirfile', type=str,
                        help='path to the workdirs file with the format <hash> <workdir> <name>. Can be generated using nextflow log')
    parser.add_argument('--output', dest='output', default='unfiltered.txt', type=str,
                        help='path to the output file')
    parser.add_argument('--work-dir', dest='workdir', default='../work', type=str,
                        help='path to the workdir directory')
    parser.add_argument('-v', dest='logging', default='INFO', type=str,
                        help='logging level')

    args = parser.parse_args()
    logging.basicConfig()
    logging.root.setLevel(args.logging)

    workdirs = common.load_working_dirs(args.workdirfile)
    inputs, outputs = common.generate_input_output_file(workdirs, work_dir_location=args.workdir)
    common.write_in_out_file(inputs, outputs, args.output)
