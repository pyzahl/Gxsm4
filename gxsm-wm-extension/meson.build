# Get the UUID from metadata.json
metadata = run_command('grep', '-E', '"uuid":', 'metadata.json', check: true)
uuid = metadata.stdout().strip().split('"')[3]

# Set the installation path based on the user-selected option
if get_option('extension-target') == 'system'
  extension_dir = join_paths(get_option('datadir'), 'gnome-shell', 'extensions', uuid)
else
  extension_dir = join_paths(get_option('home'), '.local', 'share', 'gnome-shell', 'extensions', uuid)
endif

# Install the extension files
install_data(
  ['metadata.json', 'extension.js', 'stylesheet.css'],
  install_dir: extension_dir
)
