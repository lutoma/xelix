from toposort import toposort_flatten
from pathlib import Path
from copy import copy
import time
import subprocess
import shutil
import yaml
import os

default_pkg = None
sources_dir = None
build_dir = None
image_dir = None

# FIXME
pkg_dir = (Path(__file__).parent / '..' / '..' / 'land').resolve()
root = (Path('.') / 'xpkg-build').resolve()
sources_dir = root / 'sources'
build_dir = root / 'pkgs'
image_dir = root / 'image'

for path in (root, sources_dir, build_dir, image_dir):
	try:
		os.mkdir(path)
	except FileExistsError:
		pass

with open(pkg_dir / 'default.yml') as file:
	default_pkg = yaml.safe_load(file)


class Package:
	name = None
	version = None

	config = None
	pkg_dependencies = set()
	base_dependencies = set()
	sources = list()
	patches = list()

	pkg_dir = None
	build_dir = None
	env = dict()
	cmds_configure = list()
	cmds_make = list()
	cmds_install = list()

	def __init__(self, path):
		self.config_path = path
		self.base_dependencies = set(default_pkg.get('base_dependencies', set()))

		with open(path, 'r') as file:
			self.config = yaml.safe_load(file)

		self.name = self.config['name']
		self.version = self.config['version']
		self.sources = self.config.get('sources', list())
		self.patches = self.config.get('patches', list())
		self.pkg_dependencies = set(self.config.get('dependencies', set()))

		self.cmds_configure = self.config.get('configure', list())
		self.cmds_make = self.config.get('make', list())
		self.cmds_install = self.config.get('install', list())

		self.env = {**default_pkg.get('env', dict()), **self.config.get('env', dict())}

	@property
	def pkg_dir(self):
		return self.config_path.parent

	@property
	def build_dir(self):
		return build_dir / self.name

	@property
	def dependencies(self):
		return self.base_dependencies | self.pkg_dependencies

	@property
	def last_build(self):
		try:
			with open(self.build_dir / '.xpkg-status', 'r') as file:
				return yaml.safe_load(file)['last_build']
		except FileNotFoundError:
			return 0

	@property
	def has_changed(self):
		stat = os.stat(self.config_path)
		return stat.st_mtime >= self.last_build

	def info(self, str):
		print(f'\n\033[33m{self.name}: {str}\033[m')

	def write_status(self, status):
		with open(self.build_dir / '.xpkg-status', 'w') as file:
			yaml.dump(status, file)

	def run_cmds(self, cmds, **kwargs):
		env = copy(self.env)
		env['PKGDIR'] = self.pkg_dir
		env['DESTDIR'] = image_dir

		env['SYSROOT'] = image_dir
		env['PKG_CONFIG_DIR'] = image_dir
		env['PKG_CONFIG_LIBDIR'] = f'{image_dir / "usr" / "lib"/ "pkgconfig"}:{image_dir / "usr" / "share" / "pkgconfig"}'
		env['PKG_CONFIG_SYSROOT_DIR'] = image_dir

		env['PATH'] = f'/home/lutoma/code/xelix/toolchain/local/bin/:{os.environ.get("PATH", "")}'
		env['CFLAGS'] = f'-O3'
		env['CPPFLAGS'] = '-D__STDC_ISO_10646__ -D_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC=0 -D_GLIBCXX_USE_C99_STDLIB=0'
		env['LDFLAGS'] = f'-L{image_dir / "usr" / "lib"}'

		if self.config.get('set_cflags_sysroot', True):
			env['CFLAGS'] += f' --sysroot {image_dir}'
			env['CPPFLAGS'] += f' --sysroot {image_dir}'

		if 'cwd' not in kwargs:
			kwargs['cwd'] = str(self.build_dir)

		for cmd in cmds:
			print(f'\033[36m{cmd}\033[m')
			proc = subprocess.run(cmd, shell=True, env=env, **kwargs)

			if proc.returncode != 0:
				print(f'Command {cmd} failed, aborting.')
				exit(1)

	def download(self):
		if not self.sources:
			return

		self.info(f'Downloading sources')
		dest = build_dir / self.name

		try:
			os.mkdir(dest)
		except FileExistsError:
			shutil.rmtree(dest)
			os.mkdir(dest)

		for source in self.sources:
			if 'url' in source.keys():
				filename = source['url'].split('/')[-1]
				if os.access(sources_dir / filename, os.R_OK):
					print(f'{filename} is already downloaded')
				else:
					self.run_cmds([f'wget --continue "{source["url"]}"'], cwd=sources_dir)

				tar_cmd = f'tar xf "{sources_dir / filename}" -C {dest} --strip-components=1'
				self.run_cmds([tar_cmd], cwd=sources_dir)
			elif 'dir' in source.keys():
				self.run_cmds([f'cp -r {self.pkg_dir / source["dir"]}/* {dest}'], cwd=sources_dir)

	def patch(self):
		if self.patches:
			self.info(f'Applying patches')
			skip_path = self.config.get('patch_skip_path', 1)

			patch_cmds = map(lambda x: f'patch -p{skip_path} < {self.pkg_dir}/{x}', self.patches)
			self.run_cmds(patch_cmds)

	def configure(self):
		if self.cmds_configure:
			self.info(f'Configuring')
			self.run_cmds(self.cmds_configure)

	def build(self):
		if self.cmds_make:
			self.info(f'Compiling')
			self.run_cmds(self.cmds_make)

		self.write_status({'last_build': int(time.time())})

	def install(self):
		if self.cmds_install:
			self.info(f'Installing')
			self.run_cmds(self.cmds_install)


def load_packages():
	packages = dict()
	deps = dict()

	for path in pkg_dir.glob('*/package.yml'):
		pkg = Package(path)
		packages[pkg.name] = pkg
		deps[pkg.name] = pkg.dependencies

	pkg_names = toposort_flatten(deps)
	pkgs = list(map(lambda x: packages[x], pkg_names))
	return pkgs
