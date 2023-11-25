import pytest


import app

@pytest.mark.parametrize("test_input,expected", [
    ("2023-11/17", ["2023-11", "2023-11/17"]),
    ("2023-11", ["2023-11"]),
    ("2023-11/17/18", ["2023-11", "2023-11/17", "2023-11/17/18"]),
    ("", []),
    ]
    )
def test_intermediate_path(test_input, expected):
    assert app.all_intermediate_paths(test_input) == expected



