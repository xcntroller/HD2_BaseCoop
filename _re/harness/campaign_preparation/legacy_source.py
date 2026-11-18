"""Extract current original rule bodies for differential execution, no copies.

The tiny parity harness supplies detached item/squad/profile collaborators, not
engine ABI/layout or UI. This runs the actual checked-out source algorithms.
"""
import re
from pathlib import Path


def extract(source, signature):
    start = source.index(signature)
    assert source.count(signature) == 1, signature
    # Retain source offsets while ignoring braces inside literals/comments.
    stripped = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                      lambda m: ' ' * len(m[0]), source, flags=re.S)
    begin = stripped.index('{', start)
    depth = 1
    end = begin + 1
    while depth:
        if stripped[end] == '{':
            depth += 1
        elif stripped[end] == '}':
            depth -= 1
        end += 1
    return source[start:end] + '\n'


def generate(root: Path, output: Path):
    squad = (root / 'src/game/profile_squad.cpp').read_text()
    equipment = (root / 'src/game/equipment_screen.cpp').read_text()
    functions = [extract(squad, signature) for signature in [
        'int GetItemVolume(', 'float GetSlotWeight(', 'void GetBackpackLimits(',
        'int FindStack(', 'bool SameType(', 'int TakeCount(',
        'float S_SoldierLoad::GetWeightFill(',
        'void S_SquadSoldier::PutItem(', 'int S_SquadSoldier::RemoveItem(',
        'bool S_SquadSoldier::IsEmpty(']]
    functions += [extract(equipment, signature) for signature in [
        'bool C_EquipmentMenu::IsSquadWithinWeight(', 'int C_EquipmentMenu::CountCarried(',
        'bool C_EquipmentMenu::HasRequiredItems(']]
    (output / 'legacy_bodies.inc').write_text('// Generated from current repository sources.\n' + '\n'.join(functions))
