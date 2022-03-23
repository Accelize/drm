#! /usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Copy from AWS S3 the reference designs for the parent directory
whose name matches a regular exession
"""
import sys
import argparse
import logging
import subprocess as sp
import re
import json
import os
import tempfile
from os.path import isdir, isfile, join, basename, dirname, splitext
from shutil import copyfile, rmtree
from tabulate import tabulate

AWS_S3_BUCKET = 's3://accelizecodebuildstorage'
AWS_S3_SYN_DIR = f'{AWS_S3_BUCKET}/pt_reference_design'

REGEX_HDK_DESIGNS = r'\b\d+activator_\S+'
REGEX_VITIS_DESIGNS = r'\bvitis_\S+'

DEFAULT_BITSTREAMS = (
    '2activator_axi4_250_125.json',
    'vitis_2activator_100_125.awsxclbin',
)



def run_cmd(cmd):
    ret = sp.run(cmd, shell=True, stdout=sp.PIPE, stderr=sp.STDOUT)
    logging.debug(f"Command '{cmd}' returned code '{ret.returncode}'")
    out = ret.stdout.decode("utf-8")
    if ret.returncode:
        raise RuntimeError(f'Failed to execute command: {cmd}\n{out}')
    return out


def s3_listall(s3_dir, regex='.*', regex_flags=0):
    out = run_cmd(f'aws s3 ls --recursive {s3_dir}')
    ls = [re.findall(r'(\S+)', line)[-1] for line in out.strip().split('\n')]
    ls = list(filter(lambda x: re.search(regex, x, regex_flags), ls))
    ls = list(map(lambda x: join(AWS_S3_BUCKET, x), ls))
    logging.debug(f"All files matching regex {regex} in {s3_dir}: %s" % '\n'.join(ls))
    return ls


def get_design_name(json_content):
    design_name_node = next(filter(lambda x: x['Key'] == 'Project-Name', json_content['FpgaImages'][0]['Tags']))
    assert design_name_node['Key'] == 'Project-Name'
    design_name = design_name_node['Value']
    logging.debug(f"Design name: {design_name}")
    return design_name


def get_design_type(json_content):
    design_type_node = next(filter(lambda x: x['Key'] == 'Project-Type', json_content['FpgaImages'][0]['Tags']))
    assert design_type_node['Key'] == 'Project-Type'
    design_type = design_type_node['Value']
    logging.debug(f"design type: {design_type}")
    return design_type


def check_design_version(json_content):
    design_version_string = next(filter(lambda x: x['Key'] == 'HDK-Versions', json_content['FpgaImages'][0]['Tags']))
    assert design_version_string['Key'] == 'HDK-Versions'
    design_version_list = design_version_string['Value'].split(':')
    assert len(set(design_version_list)) == 1, f"Inconsistent HDK versions: {design_version_string['Value']}"
    design_version = design_version_list[0]
    logging.debug(f"HDK version: {design_version}")
    return design_version


def copy_hdk_bitstream(s3_design_dir, dst_file, force=False):
    # Download and open AFI info file
    afi_info_file = join(s3_design_dir, 'afi_info.json')
    with tempfile.NamedTemporaryFile() as tmp_file:
        run_cmd(f'aws s3 cp {afi_info_file} {tmp_file.name}')
        logging.debug(f'Downloaded {afi_info_file} as {tmp_file.name}')
        with open(tmp_file.name, 'rt') as f:
            afi_info_json = json.load(f)
    if isfile(dst_file) and not force:
        logging.error(f"AWS Vivado bitstream '{dst_file}' is already existing: to overwrite add -f option.")
        return False
    bitstream_json = dict()
    bitstream_json['FpgaImageId'] = afi_info_json['FpgaImages'][0]['FpgaImageId']
    bitstream_json['FpgaImageGlobalId'] = afi_info_json['FpgaImages'][0]['FpgaImageGlobalId']
    # Copy AFI/AGFI json file locally in output directory
    with open(dst_file, 'wt') as f:
        json.dump(bitstream_json, f, indent=4)
    logging.info(f'Copied {dst_file}')
    return True


def copy_vitis_bitstream(s3_design_dir, dst_file, force=False):
    if isfile(dst_file) and not force:
        logging.error(f"Vitis bitstream '{dst_file}' is already existing: to overwrite add -f option.")
        return False
    # Find awsxclbin file
    bitstream_list = s3_listall(s3_design_dir, r'.*\.awsxclbin')
    assert len(bitstream_list) == 1
    # Copy the AWSXCLBIN file from S3
    src_file = bitstream_list[0]
    run_cmd(f'aws s3 cp {src_file} {dst_file}')
    logging.info(f'Copied {dst_file}')
    return True


COPY_BITSTREAM_FUNC = {
    'hdk': copy_hdk_bitstream,
    'vitis': copy_vitis_bitstream,
}


def copy_bitstreams(s3_job_dir, output, force=False):
    # Create AWS output folders
    aws_hdk_dir = join(output, 'aws_f1')
    if not isdir(aws_hdk_dir):
        os.makedirs(aws_hdk_dir)
    aws_vitis_dir = join(output, 'aws_f1')
    if not isdir(aws_vitis_dir):
        os.makedirs(aws_vitis_dir)
    # For each AFI info JSON file found
    job_version = None
    res = list()
    for afi_info_file in s3_listall(s3_job_dir, r'afi_info\.json'):
        # Download and open AFI info file
        with tempfile.NamedTemporaryFile() as tmp_file:
            run_cmd(f'aws s3 cp {afi_info_file} {tmp_file.name}')
            logging.debug(f'Downloaded {afi_info_file} as {tmp_file.name}')
            with open(tmp_file.name, 'rt') as f:
                afi_info_json = json.load(f)
        # Extract design name, design type and design version
        design_name = get_design_name(afi_info_json)
        design_type = get_design_type(afi_info_json)
        design_version = check_design_version(afi_info_json)
        if job_version is None:
            job_version = design_version
        else:
            assert job_version == design_version, f"Unexpected HDK version: current is {design_version} but expect {job_version}"
        logging.debug(f'Handling design {design_name} with HDK version {job_version}')
        # Create bitstream file depending on each project type
        is_ok = False
        if design_type == 'hdk':
            dst_file = join(aws_hdk_dir, f'{design_name}.json')
        elif design_type == 'vitis':
            dst_file = join(aws_vitis_dir, f'{design_name}.awsxclbin')
        else:
            raise RuntimeError(f'Unsupported project type {design_type}')
        try:
            s3_design_dir = dirname(afi_info_file)
            is_ok = COPY_BITSTREAM_FUNC[design_type](s3_design_dir, dst_file, force)
        except KeyboardInterrupt:
            raise
        except:
            logging.exception(f"Failed to copy {design_type} bitstream for design {design_name}")
        finally:
            res.append((dst_file, design_type, design_version, is_ok))

    if job_version is not None:
        # Copy default designs too
        for e in res:
            src_file, design_type = e[:2]
            if basename(src_file) in DEFAULT_BITSTREAMS:
                ext = splitext(src_file)[1]
                dst_file = join(dirname(src_file), f'{job_version}{ext}')
                is_ok = False
                try:
                    copyfile(src_file, dst_file)
                    logging.info(f'Copied {dst_file}')
                    is_ok = True
                except:
                    logging.exception(f"Failed to copy {design_type} bitstream for design {job_version}")
                finally:
                    res.append((dst_file, design_type, job_version, is_ok))

    return res


def list_synthesis_jobs_from_s3(regex):
    # Get the content of AWS S3 bucket
    out = run_cmd(f'aws s3 ls {AWS_S3_SYN_DIR}/')
    logging.debug(f'Content of {AWS_S3_SYN_DIR}:\n{out}')
    # Evalute regex on the list of synthesis jobs on S3 folder
    if regex is None:
        r = r'(?<=PRE)\s+(\S+)'
    else:
        r = r'(?<=PRE)\s+(\S*%s\S*)' % regex
    job_list = re.findall(r, out, re.IGNORECASE)
    # Sort list by date and time
    dated_job_list = list(map(lambda x: (extract_date_time(x), x), job_list))
    dated_job_list.sort(reverse=True)
    if len(dated_job_list) == 0:
        logging.error(f"No synthesis job found on S3 matching regex '{regex}', available jobs are:\n{out}")
    return [e[1] for e in dated_job_list]


def extract_date_time(s):
    m = re.search(r'\d{4}-\d{2}-\d{2}_\d{2}h\d{2}m\d{2}s', s)
    if m is None:
        return ''
    else:
        return m.group(0)


def download_bitstreams_from_s3(output, regex, force):
    try:
        # Check existance of output folder
        logging.debug(f'Output directory: {output}')
        if not isdir(output):
            os.makedirs(output)
        # Get synthesis jobs on AWS S3
        job_list = list_synthesis_jobs_from_s3(regex)
        if regex is None and len(job_list) > 1:
            job_list = [job_list[0]]
            logging.warning("No regex option provided by user: the most recent synthesis job will be selected")
        if len(job_list) == 0:
            return -1
        if len(job_list) > 1:
            logging.error(f"Found multiple jobs matching regex '{regex}':\n%s" % '\n'.join(job_list))
            return -1
        job_name = job_list[0][:-1]
        logging.info(f"Downloading all bitstreams in job {job_name}")
        s3_job_dir = join(AWS_S3_SYN_DIR, job_name)
        # Copy the bitstreams of all available AWS HDK synthesized designs
        result = copy_bitstreams(s3_job_dir, output, force)
    except RuntimeError as e:
        logging.error(e)

    # Show summary table
    if result:
        logging.info(f"Summary of bitstreams copied from synthesis job '{job_name}':\n\n%s\n",
                    tabulate(map(lambda x: (basename(x[0]), x[1], x[2], x[3]), result),
                                headers=['Design', 'Flow', 'HDK Version', 'Available Bitstream']))
        err = sum([not(e[2]) for e in result])
        if err == 0:
            logging.info(f'== {len(result)} designs copied ==')
        else:
            logging.error(f'== {err} designs NOT copied ==')
    else:
        logging.error('== No design copied ==')
        err = -1

    return err


## MAIN
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', dest='output', type=str, default=None, help='Specify the path to the directory to copy the designs to')
    parser.add_argument('-l', '--list', dest='list', action='store_true', default=False, help='List available synthesis jobs on S3')
    parser.add_argument('-r', '--regex', dest='regex', type=str, default=None, help=('Specify a regex pattern used to find the designs to downloads. '
                                            'If omitted, the most recent synthesis job is used'))
    parser.add_argument('-f', '--force', action='store_true', default=False, help='Overwrite bitstream file if already existing locally.')
    parser.add_argument('-c', '--clean', dest='clean', action='store_true', default=False, help='Clean output directory if existing')
    parser.add_argument('-v', dest='verbosity', action='store_true', default=False, help='More verbosity')
    args = parser.parse_args()

    level = logging.INFO
    if args.verbosity:
        level = logging.DEBUG

    logging.basicConfig(
        level=level,
        #format="%(asctime)s - %(levelname)-7s, %(lineno)4d |- %(message)s",
        format="%(levelname)-7s: %(message)s",
        handlers=[
            logging.StreamHandler()
        ])

    if args.output is None:
        args.output = "output"

    if args.list:
        job_list = list_synthesis_jobs_from_s3(args.regex)
        logging.info(f"List of synthesis jobs found on S3 matching regex '{args.regex}':\n%s" % '\n'.join(job_list))
        ret = 0
    else:
        if isdir(args.output):
            if not args.clean:
                logging.warning((f"Output directory {args.output} is already existing. "
                        "Use -c option if you want to remove it first, otherwise the bitstreams will be added to the existing one"))
            else:
                rmtree(args.output)
                logging.warning(f"Existing output directory {args.output} has been removed.")
        ret = download_bitstreams_from_s3(args.output, args.regex, args.force)

    sys.exit(ret)

