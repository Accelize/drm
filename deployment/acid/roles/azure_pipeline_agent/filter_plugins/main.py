"""Extra Ansible filters"""


def strip(text, to_strip):
    """
    Strip text.

    Args:
        text (str): Text
        to_strip (str): Text to strip.

    Returns:
        str: Striped text
    """
    return text.strip(to_strip)


class FilterModule(object):
    """Return filter plugin"""

    @staticmethod
    def filters():
        """Return filter"""
        return {'strip': strip}
