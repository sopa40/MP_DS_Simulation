import collections

import logging
from os import listdir


# Expected Format
# <hash> <workdir> <name>
# File can be generated using the 'nextflow log -f hash,workdir,name' command
def load_working_dirs(file_name):
    working_dir_by_hash = collections.defaultdict()
    with open(file_name, 'r') as input_file:
        for line in input_file:
            hash_id, workdir, name = line.strip().split('\t', 2)

            task_name = name.split(' ')
            if len(task_name) == 2:
                task_id = task_name[1]
                task_name = task_name[0]
            else:
                task_id = ''
                task_name = task_name[0]

            working_dir_by_hash[hash_id] = {
                'workdir': workdir,
                'task_name': task_name,
                'task_id': task_id,
            }

    logging.info("Loaded %d workdirs", len(working_dir_by_hash))
    return working_dir_by_hash


# Expected Format
# Generate using the read_io_files_from_log.sh script
# 1: <hash> name id (input:<path>:<size>)* (output:<path>:<size>)*
# 2: <hash> (input:<path>:<size>)* (output:<path>:<size>)*
def load_input_output_file(file_name, line_format=1):
    input_files = collections.defaultdict(lambda: collections.defaultdict())
    output_files = collections.defaultdict(lambda: collections.defaultdict())

    with open(file_name, 'r') as input_file:
        for line in input_file:

            if line_format == 1:
                hash_id, _, _, rest = line.strip().split(' ', 3)
            elif line_format == 2:
                hash_id, rest = line.strip().split(' ', 1)
            else:
                raise Exception("Invalid Format")

            files = rest.split(' ')
            try:
                for file in files:
                    in_or_out, path, size = file.split(':')
                    if in_or_out == 'input':
                        input_files[hash_id][path] = int(size)
                    elif in_or_out == 'output':
                        output_files[hash_id][path] = int(size)
                    else:
                        raise Exception("Expected input or output")
            except ValueError:
                raise Exception("Expected input or output")

    return input_files, output_files


def load_strace_files_from_working_dir(working_dir):
    files_in_dir = listdir(working_dir)
    if len(files_in_dir) == 0:
        raise Exception("No Strace Files in Working Dir")

    strace_files = [file for file in files_in_dir if file.startswith(".strace.")]
    pids = sorted([int(filename.split('.')[2]) for filename in strace_files])
    for pid in pids:
        with open(working_dir + "/.strace." + str(pid), "r") as strace_file:
            yield pid, strace_file.read()


def write_in_out_file(input_files_by_hash, output_files_by_hash, filename):
    with open(filename, 'w+') as output:
        for hash_id in input_files_by_hash.keys():
            inputs = ["input:%s:%d" % (p, int(s)) for p, s in input_files_by_hash[hash_id].items()]
            outputs = ["output:%s:%d" % (p, int(s)) for p, s in output_files_by_hash[hash_id].items()]
            output.write("%s %s %s\n" % (
                hash_id,
                ' '.join(inputs),
                ' '.join(outputs),
            ))
