import collections
import logging
from os import listdir


# Expected Format
# <hash> <workdir> <name>
# can be generated with 'nextflow log <job-name> -f hash,workdir,name'
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

    return working_dir_by_hash


# This requires access to the working dirs
def generate_input_output_file(working_dir_by_hash, work_dir_location='../work'):
    input_files_by_hash = collections.defaultdict(
        lambda: collections.defaultdict(dict))
    output_files_by_hash = collections.defaultdict(
        lambda: collections.defaultdict(dict))

    for hash_id, v in working_dir_by_hash.items():
        try:
            with open("%s/.inputs.trace" % v['workdir'].replace('/workdir', work_dir_location), "r") as file:
                for line in file:
                    path, size = line.split(":")
                    input_files_by_hash[hash_id][path] = int(size.strip())

            with open("%s/.outputs.trace" % v['workdir'].replace('/workdir', work_dir_location), "r") as file:
                for line in file:
                    path, size = line.split(":")
                    output_files_by_hash[hash_id][path] = int(size.strip())
        except:
            logging.warn("File not found %s. Maybe a failed task" % hash_id)

    return input_files_by_hash, output_files_by_hash

# Expected Format
# 1: <hash> name id (input:<path>:<size>)* (output:<path>:<size>)*
# 2: <hash> (input:<path>:<size>)* (output:<path>:<size>)*


def load_input_output_file(file_name):
    input_files = collections.defaultdict(lambda: collections.defaultdict())
    output_files = collections.defaultdict(lambda: collections.defaultdict())

    with open(file_name, 'r') as input_file:
        for line in input_file:

            hash_id, rest = line.strip().split(' ', 1)

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


def load_strace_files_from_working_dir(working_dir, work_dir_location='../work'):
    files_in_dir = listdir(working_dir.replace('/workdir', work_dir_location))
    if len(files_in_dir) == 0:
        raise Exception("No Strace Files in Working Dir")

    strace_files = [
        file for file in files_in_dir if file.startswith(".strace.")]
    pids = sorted([int(filename.split('.')[2]) for filename in strace_files])
    loggin.debug("Found %d strace files", len(pids))
    for pid in pids:
        with open(working_dir.replace('/workdir', work_dir_location) + "/.strace." + str(pid), "r") as strace_file:
            yield pid, strace_file.read()


def write_in_out_file(input_files_by_hash, output_files_by_hash, filename):
    with open(filename, 'w+') as output:
        for hash_id in input_files_by_hash.keys():
            inputs = ["input:%s:%d" % (p, int(s))
                      for p, s in input_files_by_hash[hash_id].items()]
            outputs = ["output:%s:%d" % (
                p, int(s)) for p, s in output_files_by_hash[hash_id].items()]
            output.write("%s %s %s\n" % (
                hash_id,
                ' '.join(inputs),
                ' '.join(outputs),
            ))
