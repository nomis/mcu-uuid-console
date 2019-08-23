import json
import re
import sys

def version_check():
	with open("../library.json", "r") as f:
		build_version = json.load(f)["version"]
	release_heading = re.compile(r"^(?P<version>[0-9]+\.[0-9]+\.[0-9]+)_ \|--\| (?P<date>[0-9]{4}-[0-9]{2}-[0-9]{2})$")

	for line in open("../docs/changelog.rst", "rt"):
		match = release_heading.match(line)
		if match:
			match = match.groupdict()
			if match["version"] != build_version:
				print("Latest release is " + match["version"] + " but build version is " + build_version, file=sys.stderr)
				return False
			else:
				print("Latest release is " + match["version"] + " which matches build version " + build_version)
				return True

	print("Unknown release", file=sys.stderr)
	return False

if not version_check():
	sys.exit(1)
