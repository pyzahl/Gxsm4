
# Localisation (optional)
####################################

i18n = import('i18n')
# define GETTEXT_PACKAGE
add_project_arguments('-DGETTEXT_PACKAGE="gxsm4"', language: ['c','cpp'])
i18n.gettext(meson.project_name(),
    args: '--directory=' + meson.current_source_dir()
)