from enum import StrEnum
from rich.console import Console
from beaupy import Config, confirm, prompt, select, select_multiple


Config.raise_on_interrupt = True
console = Console()
action = "start"


class Actions(StrEnum):
    INIT_PAIRING = "Init Pairing"
    WRITE_DURATION = "Write Node Duration"
    WRITE_CYCLE = "Write Node Cycle"
    SET_TIME = "Set Time"
    SET_PHASE = "Set Next Phase"
    GET_TIME = "Get Clock Times"
    EXIT = "Exit"


actions = [x.value for x in list(Actions)]

try:
    while action != "Exit":
        console.print("Select Action")
        # Choose one item from a list
        action = select(actions, cursor="->", cursor_style="cyan")
        match action:
            case Actions.INIT_PAIRING.value:
                
except KeyboardInterrupt:
    print("\nProgram terminated by user.")


print("Goodbye")
