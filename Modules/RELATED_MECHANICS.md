# Related Existing Mechanics — Integration Register

These systems live elsewhere in the project/repository or in broader designs. This track defines integration seams; it must not duplicate their canonical state owners.

| System family | Existing code area examples | Relationship | Boundary rule |
|---|---|---|---|
| Trees, logs, woodworking | HarvestableActor, CarvableLog, Wood, Phase1_Physical–Phase6_Utility | Source lumber for uprights/boards/struts; some objects are physical carryables. | Physical log state is not a stackable inventory item. |
| Stone/stone masonry | StoneCore, StonePhaseA–G, StoneMasonryActor | Source material and physical stones can become placed structure pieces. | Only explicitly supported stone states are persistent; do not imply every carried/dragged/fractured state is saved. |
| Inventory/crafting | InventoryCore, InventoryWorld, InventoryCrafting, InventoryBridge, InventoryProducts, InventoryUI | Tools and standard pieces/materials flow to/from world objects. | One canonical inventory owner; resource debit and placement use transaction semantics. |
| Material interactions M1–M6 | MaterialCore, MaterialPhaseM1–M6 | Supplies strength, moisture, temperature, fire/water/impact response. | StructuralIntegrity consumes normalized properties; it does not duplicate material logic. |
| Fire/campfire/burning | BurnSubsystem, Campfire, FireStone, MaterialPhaseM2 | Heat/damage can affect durability and survival. | Fire emits typed damage/environment events. |
| Traps/defense/raid | Phase6_Utility, RaidCore, RaidPhaseR1–R9, StickPhaseS5–S6 | May damage structures and interact with routes. | Raid AI/navigation remain outside excavation-core ownership. |
| Crops/soil | PhaseT2_Plant–PhaseT5_Harvest, WorldPhaseW4_WaterSoil | Environmental moisture and excavation may affect growing areas. | Crop/soil state is not voxel density. |
| Animals/production/reproduction | PhaseT6_Animal–PhaseT9_Integration | Daily growth, feed and products interact with world stocks. | One canonical stock ledger and update authority per system. |
| World chunks/events/water/decay/chemistry | WorldCore, WorldPhaseW1–W6, SimCore | Provides environment/time events and state. | Chunk/environment state is not an alias for terrain density. |
| Exploration/journal/time/weather | Broader game systems/plans | Can consume discovery/world events. | Optional observers do not join edit transactions unless declared. |

This table identifies likely integration seams, not proof that adapters already exist. Each connection requires an owner, contract tests, migration plan and execution evidence.
