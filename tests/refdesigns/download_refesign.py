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
from os.path import isdir, isfile, join, basename
from shutil import copyfile, rmtree
from tabulate import tabulate


AWS_S3_BUCKET = "s3://accelizecodebuildstorage/pt_reference_design"

REGEX_HDK_DESIGNS = r'\b\d+activator_\S+'
REGEX_VITIS_DESIGNS = r'\bvitis_\S+'

HDK_DEFAULT_BITSTREAM = '2activator_axi4_250_125'
VITIS_DEFAULT_BITSTREAM = 'vitis_2activator_100_125'


def run_cmd(cmd):
    ret = sp.run(cmd, shell=True, stdout=sp.PIPE, stderr=sp.STDOUT)
    logging.debug(f"Command '{cmd}' returned code '{ret.returncode}'")
    out = ret.stdout.decode("utf-8")
    if ret.returncode:
        raise RuntimeError(f'Failed to execute command: {cmd}\n{out}')
    return out


def get_hdk_version(s3_design_dir):
    with tempfile.NamedTemporaryFile() as tmp_file:
        # Copy the AFI info file from S3
        src_file = join(s3_design_dir, 'IMPL', 'AFI', 'afi_info.json')
        cmd = f'aws s3 cp {src_file} {tmp_file.name}'
        run_cmd(cmd)
        logging.debug(f'Downloaded {src_file} as {tmp_file.name}')
        # Parsing AFI info file to extract HDK-Versions tag
        with open(tmp_file.name, 'rt') as f:
            content = json.load(f)
        hdk_version_string = next(filter(lambda x: x['Key'] == 'HDK-Versions', content['FpgaImages'][0]['Tags']))
        assert hdk_version_string['Key'] == 'HDK-Versions'
        hdk_version_list = hdk_version_string['Value'].split(':')
        assert len(set(hdk_version_list)) == 1, f"Inconsistent HDK versions: {hdk_version_string['Value']}"
        hdk_version = hdk_version_list[0]
        logging.debug(f"HDK version: {hdk_version}")
        return hdk_version


def copy_aws_hdk_bitstream(s3_job_dir, output, force=False):
    # Find all the designs performed by this synthesis job
    cmd = f'aws s3 ls {s3_job_dir}/aws/'
    out = run_cmd(cmd)
    logging.debug(f"List of synthesis for job '{s3_job_dir}':\n{out}")
    # Create AWS HDK output folder
    out_dir = join(output, 'aws_f1')
    if not isdir(out_dir):
        os.makedirs(out_dir)
    # Get AFI/AGFI for each AWS HDK design
    hdk_version = None
    res = list()
    for d in re.finditer(REGEX_HDK_DESIGNS, out, re.IGNORECASE):
        design_name = d.group(0)[:-1]
        is_ok = False
        logging.debug(f"Parsing design {design_name}")
        dst_file = join(out_dir, f'{design_name}.json')
        try:
            if isfile(dst_file) and not force:
                logging.error(f"AWS Vivado bitstream '{dst_file}' is already existing: to overwrite add -f option.")
                continue
            # Check HDK version
            current_hdk_version = get_hdk_version(join(s3_job_dir, 'aws', design_name))
            if hdk_version is None:
                hdk_version = current_hdk_version
            else:
                assert hdk_version == current_hdk_version, f"Unexpected HDK version: current is {current_hdk_version} but expect {hdk_version}"
            # Find and copy design bitstream
            with tempfile.NamedTemporaryFile() as tmp_file:
                # Copy the AFI info file from S3
                src_file = join(s3_job_dir, 'aws', design_name, 'IMPL', 'AFI', 'afi_info.json')
                cmd = f'aws s3 cp {src_file} {tmp_file.name}'
                run_cmd(cmd)
                logging.debug(f'Downloaded {src_file} as {tmp_file.name}')
                # Parsing AFI info file to extract AFI/AGFI IDs
                with open(tmp_file.name, 'rt') as f:
                    content = json.load(f)
                new_content = dict()
                new_content['afi'] = content['FpgaImages'][0]['FpgaImageId']
                new_content['agfi'] = content['FpgaImages'][0]['FpgaImageGlobalId']
                if HDK_DEFAULT_BITSTREAM in design_name:
                    hdk_version = next(filter(lambda x: x['Key'] == 'HDK-Versions', content['FpgaImages'][0]['Tags']))
                    assert hdk_version['Key'] == 'HDK-Versions'
                    hdk_version = hdk_version['Value'].split(':')[0]
                    logging.debug(f"Found HDK version: {hdk_version}")
                # Copy AFI/AGFI json file locally in output directory
                with open(dst_file, 'wt') as f:
                    json.dump(new_content, f, indent=4)
                logging.info(f'Copied {dst_file}')
                is_ok = True
        except KeyboardInterrupt:
            raise
        except:
            logging.exception(f"Failed to copy HDK bitstream for design: {basename(dst_file)}")
        finally:
            res.append((basename(dst_file), 'HDK', is_ok))

    if hdk_version is not None:
        # Copy default design too
        src_file = join(out_dir, f'{HDK_DEFAULT_BITSTREAM}.json')
        dst_file = join(out_dir, f'{hdk_version}.json')
        is_ok = False
        try:
            copyfile(src_file, dst_file)
            logging.info(f'Copied {dst_file}')
            is_ok = True
        except:
            logging.exception(f"Failed to copy HDK bitstream for design: {basename(dst_file)}")
        finally:
            res.append((basename(dst_file), 'HDK', is_ok))

    return res


def copy_aws_vitis_bitstream(s3_job_dir, output, force=False):
    # Find all the designs performed by this synthesis job
    cmd = f'aws s3 ls {s3_job_dir}/aws/'
    out = run_cmd(cmd)
    logging.debug(f"List of synthesis for job '{s3_job_dir}':\n{out}")
    # Create AWS HDK output folder
    out_dir = join(output, 'aws_xrt')
    if not isdir(out_dir):
        os.makedirs(out_dir)
    # Get AFI/AGFI for each AWS HDK design
    hdk_version = None
    res = list()
    for d in re.finditer(REGEX_VITIS_DESIGNS, out, re.IGNORECASE):
        design_name = d.group(0)[:-1]
        is_ok = False
        logging.debug(f"Parsing design {design_name}")
        dst_file = join(out_dir, f'{design_name}.awsxclbin')
        try:
            if isfile(dst_file) and not force:
                logging.error(f"Vitis bitstream '{dst_file}' is already existing: to overwrite add -f option.")
                continue
            # Check HDK version
            current_hdk_version = get_hdk_version(join(s3_job_dir, 'aws', design_name))
            if hdk_version is None:
                hdk_version = current_hdk_version
            else:
                assert hdk_version == current_hdk_version, f"Unexpected HDK version: current is {current_hdk_version} but expect {hdk_version}"
            # Copy the AWSXCLBIN file from S3
            src_file = join(s3_job_dir, 'aws', design_name, 'IMPL', 'AFI', f'{design_name}.awsxclbin')
            cmd = f'aws s3 cp {src_file} {dst_file}'
            run_cmd(cmd)
            logging.info(f'Copied {dst_file}')
            is_ok = True
        except KeyboardInterrupt:
            raise
        except:
            logging.exception(f"Failed to copy Vitis bitstream for design: {design_name}")
        finally:
            res.append((basename(dst_file), 'Vitis', is_ok))

    if hdk_version is not None:
        # Copy default design too
        src_file = join(out_dir, f'{VITIS_DEFAULT_BITSTREAM}.awsxclbin')
        dst_file = join(out_dir, f'{hdk_version}.awsxclbin')
        is_ok = False
        try:
            copyfile(src_file, dst_file)
            logging.info(f'Copied {dst_file}')
            is_ok = True
        except:
            logging.exception(f"Failed to copy Vitis bitstream for design: {basename(dst_file)}")
        finally:
            res.append((basename(dst_file), 'Vitis', is_ok))

    return res


def list_synthesis_jobs_from_s3(regex):
    # Get the content of AWS S3 bucket
    cmd = f'aws s3 ls {AWS_S3_BUCKET}/'
    out = run_cmd(cmd)
    logging.debug(f'Content of {AWS_S3_BUCKET}:\n{out}')
    # Evalute regex on the list of synthesis jobs on S3 folder
    if regex is None:
        r = r'(?<=PRE)\s+(\S+)'
    else:
        r = r'(?<=PRE)\s+(\S+%s\S+)' % regex
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


def download_bitstream_from_s3(output, regex, force):
    try:
        # Check existance of output dir
        logging.debug(f'Output directory: {output}')
        if not isdir(args.output):
            os.makedirs(args.output)
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
        logging.info(f"Downloading all bitstreams from job {job_name}")
        s3_job_dir = join(AWS_S3_BUCKET, job_name)
        result = []
        # Copy the bitstreams of all available AWS HDK synthesized designs
        result += copy_aws_hdk_bitstream(s3_job_dir, output, force)
        # Copy the bitstreams of all available AWS HDK synthesized designs
        result += copy_aws_vitis_bitstream(s3_job_dir, output, force)
    except RuntimeError as e:
        logging.error(e)

    # Show summary table
    logging.info(f"Summary of bitstreams copied from synthesis job '{job_name}':\n\n%s\n",
                tabulate(result, headers=['Design', 'Flow', 'Available Bitstream']))
    err = sum([not(e[2]) for e in result])
    if err == 0:
        logging.info('== All designs copied ==')
    else:
        logging.error(f'== {err} designs NOT copied ==')

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

    if isdir(args.output):
        if not args.clean:
            logging.warning((f"Output directory {args.output} is already existing. "
                    "Use -c option if you want to remove it first, otherwise the bitstreams will be added to the existing one"))
        else:
            rmtree(args.output)
            logging.warning(f"Existing output directory {args.output} has been removed.")

    if args.list:
        job_list = list_synthesis_jobs_from_s3(args.regex)
        logging.info(f"List of synthesis jobs found on S3 matching regex '{args.regex}':\n%s" % '\n'.join(job_list))
        ret = 0
    else:
        ret = download_bitstream_from_s3(args.output, args.regex, args.force)

    sys.exit(ret)

