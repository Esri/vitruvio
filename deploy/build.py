import argparse
import glob
import subprocess
import json
import shutil
import os
from pathlib import Path
from string import Template

# Dirs and Paths
SCRIPT_DIR = Path(__file__).resolve().parent
CFG_PATH = SCRIPT_DIR / 'installer_config.json'
RESOURCE_DIR = SCRIPT_DIR / 'resources'

DEFAULT_BUILD_DIR = SCRIPT_DIR / 'build_msi'

TEMPLATE_DIR = SCRIPT_DIR / 'templates'
DEPLOYMENT_PROPERTIES_TEMPLATE_PATH = TEMPLATE_DIR / 'deployment.properties.in'

WIX_TEMPLATE_PATH = TEMPLATE_DIR / 'vitruvio.wxs.in'

PROJECT_NAME = 'Vitruvio'
PLATFORM = 'win64'
PACKAGE_VENDOR_ESCAPED = 'Esri R&amp;amp;D Center Zurich'


def get_version_string(includePre=False, build_version=''):
	version_map = {}
	delimiter = '.'
	with open(CFG_PATH, 'r') as version_file:
		version_map = json.load(version_file)['VitruvioVersion']
	version_string = version_map['major'] + delimiter + \
            version_map['minor']
	if len(build_version) > 0:
		version_string += '+b' + build_version
	return version_string


def gen_installer_filename(build_version):
	return PROJECT_NAME + '-installer-' + get_version_string(True, build_version) + '.msi'


def rel_to_os_dir(path):
	return Path(path).absolute()


def clear_folder(path):
	if path.exists():
		shutil.rmtree(path)
	os.makedirs(path)


def run_wix_command(cmd, build_dir):
	process = subprocess.run(args=[str(i) for i in cmd], cwd=build_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          universal_newlines=True)
	print(process.args)
	print(process.stdout)
	print(process.stderr)


def wix_harvest(unreal_version, binaries_install_dir, build_dir):
	output_path = build_dir / f'vitruvio_harvest.wxs'
	wxs_file = output_path
	wxs_var = f'-dVitruvioInstallComponentDir={binaries_install_dir}'
	heat_cmd = ['heat', 'dir', binaries_install_dir, '-dr', f'VitruvioInstallComponentDir', '-ag',
             '-cg', f'VitruvioInstallComponent', '-sfrag', '-template fragment', '-var', f'wix.VitruvioInstallComponentDir', '-srd', '-sreg', '-o', output_path]

	run_wix_command(heat_cmd, build_dir)

	return wxs_file, wxs_var


def wix_compile(sources, vars, build_dir):
	candle_cmd = ['candle', '-arch', 'x64',
               build_dir / 'vitruvio.wxs'] + sources + vars
	run_wix_command(candle_cmd, build_dir)

	objects = glob.glob(str(build_dir / '*.wixobj'))
	return objects


def wix_link(objects, vars, build_dir, installer_filename):
	light_cmd = ['light', f'-dWixUILicenseRtf={SCRIPT_DIR}\\resources\\license.rtf', '-dInstallDir=', '-ext', 'WixUIExtension',
              '-cultures:en-us', '-ext', 'WixUtilExtension', '-o', f'{build_dir}\\' + installer_filename]
	light_cmd.extend(objects)
	light_cmd.extend(vars)
	run_wix_command(light_cmd, build_dir)


def create_installer(unreal_version, binary_folder, build_dir, installer_filename):
	wxs_files = []
	wxs_vars = []

	major = unreal_version['major']
	minor = unreal_version['minor']

	binary_path = rel_to_os_dir(binary_folder)

	print(binary_path, " ", binary_folder)
	wxs_file, wxs_var = wix_harvest(
		str(major) + "_" + str(minor), binary_path, build_dir)
	wxs_files.append(wxs_file)
	wxs_vars.append(wxs_var)

	wix_objects = wix_compile(wxs_files, wxs_vars, build_dir)
	wix_link(wix_objects, wxs_vars, build_dir, installer_filename)


def copy_binaries(binary_folder, build_dir):
	clear_folder(build_dir)

	os_src_path = rel_to_os_dir(binary_folder)
	if os_src_path.exists():
		binary_path = build_dir / 'install' / 'Vitruvio'
		print("copy files from ", os_src_path, " to ", binary_path)
		shutil.copytree(os_src_path, binary_path)
		return binary_path

	return Path("")


def fill_template(src_path, token_value_dict):
	result = ''
	with open(src_path, 'r') as file:
		src = Template(file.read())
		result = src.substitute(token_value_dict)
	return result


def fill_template_to_file(src_path, dest_path, token_value_dict):
	result = fill_template(src_path, token_value_dict)
	with open(dest_path, 'w') as outfile:
		outfile.write(result)


def fill_wix_template(unreal_version, copied_binaries, build_dir):
	wix_properties = {
		'PROJECT_NAME': PROJECT_NAME,
		'VITRUVIO_VERSION_MMP': get_version_string(),
		'PACKAGE_VENDOR_ESCAPED': PACKAGE_VENDOR_ESCAPED,
		'RESOURCE_FOLDER': RESOURCE_DIR,
		'UNREAL_MAJOR': unreal_version['major'],
		'UNREAL_MINOR': unreal_version['minor']
	}
	fill_template_to_file(WIX_TEMPLATE_PATH, build_dir /
	                      'vitruvio.wxs', wix_properties)


def fill_deployment_properties_templates(installer_filename, build_dir, build_version):
	deployment_properties = {
		'PACKAGE_NAME': PROJECT_NAME,
		'VITRUVIO_VERSION_BASE': get_version_string(),
		'VITRUVIO_VERSION': get_version_string(True, build_version),
		'PACKAGE_FILE_NAME': installer_filename,
		'VITRUVIO_PKG_OS': PLATFORM
	}
	fill_template_to_file(DEPLOYMENT_PROPERTIES_TEMPLATE_PATH, build_dir /
                       'deployment.properties', deployment_properties)


def parse_arguments():
	parser = argparse.ArgumentParser(
		description='Build unified MSI installer for the Vitruvio Plugin')

	parser.add_argument('-bin', '--binaries', default='',
						help='path to Vitruvio binaries')

	parser.add_argument('-bv', '--build_version', default='',
	                    help='build version for current Vitruvio build')

	parser.add_argument('-bd', '--build_dir', default=str(DEFAULT_BUILD_DIR),
	                    help='build directory where installer files are generated')

	parsed_args = vars(parser.parse_args())

	return Path(parsed_args['binaries']).resolve(), parsed_args['build_version'], Path(parsed_args['build_dir']).resolve()


def main():
	binary_folder, build_version, build_dir = parse_arguments()

	with open(CFG_PATH, 'r') as config_file:
		installer_config = json.load(config_file)

	unreal_version = installer_config["UnrealVersion"]
	
	copied_binaries = copy_binaries(binary_folder, build_dir)
	installer_filename = gen_installer_filename(build_version)

	fill_deployment_properties_templates(
		installer_filename, build_dir, build_version)
	fill_wix_template(unreal_version, copied_binaries, build_dir)

	create_installer(unreal_version, copied_binaries,
	                 build_dir, installer_filename)


if __name__ == '__main__':
	main()
