"""
This script is used to convert an architecture file to a tensil model.
Save :
    - logs of the compilation in a log file
    - architecture files (3 verilog files)
"""

import docker
import os
import argparse
from pathlib import Path

def save_compilation_result(logs, name, path):
    """
    Save the logs in a log file
    """
    print("logs in:", path+name+".log")

    with open(path+name+".log","wb") as file:
        file.write(logs)


def arch_to_rtl(args):
    # Create output directory
    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)

    # Network Compilation
    pwd = os.getcwd()
    try:
        client = docker.from_env()
    except docker.errors.DockerException as er:
        raise docker.errors.DockerException("Error when initializing docker client, maybe it's not launch ?") from er#
    
    arch_name = args.arch_path.stem
    try:
        print("Tensil rtl generation...")
        log_rtl = client.containers.run("tensilai/tensil:latest",
                                        ["tensil", "rtl", "-a", args.arch_path.as_posix(), "-d", "64", "-t", args.output_dir, "-s", "true" ],
                                        volumes=[pwd + ":/work"],
                                        working_dir="/work",
                                        stderr=False)
        save_compilation_result(log_rtl, arch_name+"_rtl", args.output_dir)

        print("------ RTL generation successful! ------")


    except docker.errors.ContainerError as exc:
        with open(args.output_dir + arch_name + ".log","wb") as file:
            file.write(exc.container.logs())
        print("------ Compilation failed -------")
        print("[ERROR] ",exc.container.logs())


if __name__ == "__main__":
    # Define the command line arguments for the script
    parser = argparse.ArgumentParser()
    parser.add_argument('--arch-path', '-a', type=Path, default= "arch/zyboz7.tarch", help='Path to tensil architecture file')
    parser.add_argument('--output-dir', '-d', type=str, default= "tensil/", help='Output directory')
    args = parser.parse_args()

    arch_to_rtl(args)
