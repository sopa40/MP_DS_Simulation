# Merge the Perf Trace files with the filtered I/O File from the Strace execution

import argparse
import logging
import collections
import common

import re


def hash_by_task_name(working_dir_by_hash):
    result = collections.defaultdict(lambda: collections.defaultdict(lambda: list()))
    for hash_id, v in working_dir_by_hash.items():
        result[v['task_name']][v['task_id']].append(hash_id)

    return result


working_dir_pattern = re.compile(r"/[0-9a-z]{2}/[0-9a-z]{30}", flags=re.MULTILINE)
tmp_dir_pattern = re.compile(r"/tmp/nxf\.[a-zA-Z0-9]{10}", flags=re.MULTILINE)


def reduce_to_basename(files_by_hash):
    result = {}
    for hash_id, v in files_by_hash.items():

        result[hash_id] = {
            'paths': set(),
            'paths_with_sizes': set()
        }

        for path, size in v.items():
            path = working_dir_pattern.sub("", path)
            path = tmp_dir_pattern.sub("", path)
            result[hash_id]['paths'].add(path)
            result[hash_id]['paths_with_sizes'].add((path, size))
    return result


def rewrite_dirs(files, workdir):
    result = {}
    for path, size in files.items():
        path = working_dir_pattern.sub(workdir, path)
        path = tmp_dir_pattern.sub(workdir, path)
        result[path] = size

    return result


def find_match_with_exact_size(strace_files, possible_matches, perf_input_basename):
    return [perf_id for perf_id in possible_matches if
            perf_input_basename[perf_id]['paths_with_sizes'] >= strace_files['paths_with_sizes']]


def find_match_ignoring_size(strace_files, possible_matches, perf_input_basename):
    return [perf_id for perf_id in possible_matches if perf_input_basename[perf_id]['paths'] >= strace_files['paths']]


def find_match_with_least_changes(strace_files, possible_matches, perf_input_basename):
    count_mismatches = [(perf_id, len(strace_files['paths'] - perf_input_basename[perf_id]['paths'])) for perf_id in
                        possible_matches]

    least_changes = sorted(count_mismatches, key=lambda p1: p1[1])
    print([least_changes[0][0]])
    return [least_changes[0][0]]


def match_perf_and_strace_run(strace_workdirs, perf_workdirs, strace_filtered_io_file, perf_unfiltered_io_file):
    result_input_files = collections.defaultdict()
    result_output_files = collections.defaultdict()

    strace_workdirs = common.load_working_dirs(strace_workdirs)
    perf_workdirs = common.load_working_dirs(perf_workdirs)

    strace_hash_by_task_name = hash_by_task_name(strace_workdirs)
    perf_hash_by_task_name = hash_by_task_name(perf_workdirs)
    
    print(strace_filtered_io_file)
    strace_input_files, strace_output_files = common.load_input_output_file(strace_filtered_io_file, line_format=1)
    perf_input_files, perf_output_files = common.load_input_output_file(perf_unfiltered_io_file, line_format=1)

    print(strace_input_files['9f/5ac9ac'])
    strace_input_basename, strace_output_basename = reduce_to_basename(strace_input_files), reduce_to_basename(
        strace_output_files)
    print(strace_input_basename['9f/5ac9ac'])
    perf_input_basename, perf_output_basename = reduce_to_basename(perf_input_files), reduce_to_basename(
        perf_output_files)

    used_strace = set()
    used_perf = set()

    for task_name, v in strace_hash_by_task_name.items():
        for task_id, hash_ids in v.items():
            possible_matches = perf_hash_by_task_name[task_name][task_id]
            if len(possible_matches) != len(hash_ids):
                raise Exception("Different Number of Tasks with the Same TaskID")

            for strace_id in hash_ids:
                if strace_id in used_strace:
                    continue

                possible_matches = list(set(possible_matches) - used_perf)
                strace_sets = strace_input_basename[strace_id]
                matches = None

                matches = find_match_with_exact_size(strace_sets, possible_matches, perf_input_basename)
                if len(matches) == 0:
                    matches = find_match_ignoring_size(strace_sets, possible_matches, perf_input_basename)
                    if len(matches) == 1:
                        logging.info("Merging with differing filesizes")

                if len(matches) == 0:
                    matches = find_match_with_least_changes(strace_sets, possible_matches, perf_input_basename)
                if len(matches) == 0:
                    raise Exception('Cant merge %s with anything' % strace_id)

                if len(matches) > 1:
                    raise Exception('Ambiguous machtes when merging %s, (%s)' % (strace_id, ','.join(matches)))

                used_strace.add(strace_id)
                used_perf.add(matches[0])

                print("merging %s with %s" % (strace_id, matches[0]))
                result_input_files[matches[0]] = rewrite_dirs(strace_input_files[strace_id],
                                                              perf_workdirs[matches[0]]['workdir'])
                result_output_files[matches[0]] = rewrite_dirs(strace_output_files[strace_id],
                                                               perf_workdirs[matches[0]]['workdir'])
                break

    return result_input_files, result_output_files


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Merge the Perf trace with the Strace io_file')
    parser.add_argument('strace_workdirs', type=str,
                        help='path to the strace workdir file with the format <hash> <workdir> <name>. Can be generated using nextflow log')
    parser.add_argument('perf_workdirs', type=str,
                        help='path to the perf workdir file with the format <hash> <workdir> <name>. Can be generated using nextflow log')
    parser.add_argument('strace_io_file', type=str,
                        help='path to the FILTERED strace io file workdir file with the format <hash> <workdir> <name>. Can be generated using nextflow log')
    parser.add_argument('perf_unfiltered_io_file', type=str,
                        help='path to the UNFILTERED perf io file workdir file with the format <hash> <workdir> <name>. Can be generated using nextflow log')


    parser.add_argument('--output', dest='output', default='merged_io_files.txt', type=str,
                        help='path to the output file')
    parser.add_argument('-v', dest='logging', default='INFO', type=str,
                        help='logging level')

    args = parser.parse_args()


    logging.basicConfig()
    logging.root.setLevel(args.logging)
    
    in_file, out_file = match_perf_and_strace_run(args.strace_workdirs, args.perf_workdirs, args.strace_io_file, args.perf_unfiltered_io_file)
    common.write_in_out_file(in_file, out_file, args.output)
