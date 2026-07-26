class ColleiError(RuntimeError):
    """An expected user-facing collei failure."""


class ColleiHelp(Exception):
    """Raised to display help text and exit cleanly."""


class CommandError(ColleiError):
    """An external command failed."""


class UnsupportedNativeConfiguration(ColleiError):
    """当前 Python 启动流程不支持该 VM 配置。"""
