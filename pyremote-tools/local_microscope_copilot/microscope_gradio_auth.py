"""Authentication helpers for the GXSM4 Gradio GUI."""

import os
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_AUTH_USER = "microscope"
DEFAULT_AUTH_PASSWORD_ENV = "MICROSCOPE_COPILOT_PASSWORD"
DEFAULT_AUTH_PASSWORD_FILE = SCRIPT_DIR / ".microscope_gui_password"


def resolve_gradio_auth(
    username=DEFAULT_AUTH_USER,
    password_env=DEFAULT_AUTH_PASSWORD_ENV,
    password_file=DEFAULT_AUTH_PASSWORD_FILE,
    require_auth=False,
):
    """
    Resolve optional Gradio basic login credentials.

    Password precedence:
    1. environment variable, default MICROSCOPE_COPILOT_PASSWORD
    2. local password file, default .microscope_gui_password
    """
    username = str(username or DEFAULT_AUTH_USER)
    password_env = str(password_env or DEFAULT_AUTH_PASSWORD_ENV)
    password_file = Path(password_file or DEFAULT_AUTH_PASSWORD_FILE)
    password = os.environ.get(password_env, "")
    source = ""
    if password:
        source = "environment:{}".format(password_env)
    elif password_file.exists():
        password = password_file.read_text().splitlines()[0].strip()
        source = "file:{}".format(password_file)
    if not password:
        if require_auth:
            raise SystemExit(
                "Microscope GUI auth is required, but no password was found. "
                "Set {} or create {}.".format(password_env, password_file)
            )
        return None, {
            "enabled": False,
            "username": username,
            "password_source": "",
            "note": (
                "Authentication disabled. Set {} or create {} to enable it."
            ).format(password_env, password_file),
        }
    return (username, password), {
        "enabled": True,
        "username": username,
        "password_source": source,
        "note": "Gradio login enabled for user '{}'.".format(username),
    }
