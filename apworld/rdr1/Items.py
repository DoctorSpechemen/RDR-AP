from typing import Dict, NamedTuple

from BaseClasses import Item, ItemClassification

from .Data import (OUTFITS, ANIMAL_TRAPS, BASE_ID, CLUSTER_NAMES, DEAD_EYE_DRAIN_TRAP, DEAD_EYE_ITEM, FILLER_ITEMS, GAME_NAME, REMOVED_FILLERS, STORY_LOCKED_OUTFITS, CONSUMABLES,
                   DRUNK_TRAP, EXTRA_TRAPS, GANG_TRAP, RANDOM_ANIMAL_TRAP, WEAPONS)


class RDRItem(Item):
    game = GAME_NAME


class ItemData(NamedTuple):
    code: int
    classification: ItemClassification


def _build() -> Dict[str, ItemData]:
    table: Dict[str, ItemData] = {}
    code = BASE_ID

    def add(name: str, classification: ItemClassification) -> None:
        nonlocal code
        table[name] = ItemData(code, classification)
        code += 1

    code += len(CLUSTER_NAMES)  # ids of the removed mission cluster items stay reserved, so later ids do not move
    add(DEAD_EYE_ITEM, ItemClassification.progression)
    for name in WEAPONS:
        add(name, ItemClassification.useful)
    for name in FILLER_ITEMS:
        add(name, ItemClassification.filler)
    code += REMOVED_FILLERS
    for name in OUTFITS:
        add(name, ItemClassification.filler)
    for name in ANIMAL_TRAPS + [RANDOM_ANIMAL_TRAP, GANG_TRAP, DEAD_EYE_DRAIN_TRAP, DRUNK_TRAP] + EXTRA_TRAPS:
        add(name, ItemClassification.trap)
    code += 1   # id of the removed Ricketts joke trap
    for name in CONSUMABLES:
        add(name, ItemClassification.filler)
    # Event item used for the goal; has no id.
    return table


item_table: Dict[str, ItemData] = _build()
item_name_to_id: Dict[str, int] = {name: data.code for name, data in item_table.items()}

item_name_groups = {
    "Weapons": set(WEAPONS),
    "Traps": set(ANIMAL_TRAPS + [RANDOM_ANIMAL_TRAP, GANG_TRAP, DEAD_EYE_DRAIN_TRAP, DRUNK_TRAP] + EXTRA_TRAPS),
    "Animal Ambush Traps": set(ANIMAL_TRAPS + [RANDOM_ANIMAL_TRAP]),
    "Filler": set(FILLER_ITEMS) | set(CONSUMABLES),
    "Consumables": set(CONSUMABLES),
    "Outfits": set(OUTFITS) - STORY_LOCKED_OUTFITS,
}
