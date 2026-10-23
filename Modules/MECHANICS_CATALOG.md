# Complete Mechanics Catalog

This is the mechanic-by-mechanic inventory for the connected nine-phase world system. It defines scope and owner; rows are not a claim that every behavior is already implemented.

## A. Shared contracts and runtime profile
| Mechanic | Owner | Required behavior |
|---|---|---|
| LOW_MID/HIGH_END profiles | RuntimeProfiles | Grid resolution, active chunks, async jobs, queues and per-frame publish budgets are validated. |
| Session grid lock | RuntimeProfiles | Grid dimensions cannot silently change after authoritative terrain data exists. |
| Stable IDs/revisions | CoreContracts | Entity, plan, operation, region and snapshot IDs remain unambiguous; revisions invalidate stale work. |
| Request validation | CoreContracts | Invalid ranges, dimensions, enum values, buffer sizes and references fail explicitly. |
| Units/coordinates | CoreContracts | Meters/centimeters and world/local/cell/chunk conversions are explicit at boundaries. |

## B. World scanning and classification
| Mechanic | Owner | Required behavior |
|---|---|---|
| Raycast | WorldScan | Find the first eligible hit under filters/query policy. |
| Shape trace | WorldScan | Sweep sphere/box/capsule-like query shapes along a bounded path. |
| Overlap | WorldScan | Return eligible entities inside a requested bounded volume. |
| Surface sampling | WorldScan | Sample height, normal, slope and continuity across a local grid. |
| Bounds/physics observation | WorldScan | Return bounds and supported physics facts on request. |
| Hit analysis | WorldScan | Keep raw hit facts separate from semantic capability classification. |
| Tags/layer markers | WorldClassification | Explicit actor/component tags and trusted Landscape/physical-material markers outrank inference. |
| Manual region/AABB | WorldClassification | Explicit authored override has deterministic priority. |
| Region grid/revision | WorldClassification | Local classification includes source provenance and map revision. |
| Landscape Builder | WorldClassification | Batch-build/update dirty areas rather than scanning the entire world every frame. |
| Region types | WorldClassification | EditableVoxel, StaticGeometry, Transition, Water and Unknown stay distinct. |

## C. Editable terrain and derived geometry
| Mechanic | Owner | Required behavior |
|---|---|---|
| Base-density source | TerrainVoxel | Sample Landscape-derived density or deterministic flat/test source. |
| Sparse density overrides | TerrainVoxel | Store changed density/material samples relative to base; mesh is not authoritative data. |
| Sphere/capsule/box edit | TerrainVoxel + Excavation | Apply bounded dig/fill edits only after validation. |
| Cell/chunk addressing | TerrainVoxel | Deterministic conversion and affected-neighbor calculation. |
| Edit transaction/rollback | Excavation + TerrainVoxel | Preflight first; failed edits leave no partial changes. |
| Revisioned snapshots | TerrainVoxel | Export deterministic bordered density/material snapshots. |
| Smooth mesh generation | MeshCollision | CPU Marching Tetrahedra or replaceable mesher consumes immutable snapshots. |
| Vertices/normals/UV/material IDs | MeshCollision | Validate topology, normals, material assignments and index buffers. |
| Dirty/full-refresh queue | MeshCollision | Coalesce edits and bound the queue. |
| Stale async-job rejection | MeshCollision | Never publish mesh from an obsolete density revision. |
| Collision/navigation refresh | MeshCollision | Track derived revisions separately and respect budgets. |
| Chunk seams | TerrainVoxel + MeshCollision | Border samples and neighbor invalidation prevent holes/gaps. |

## D. Placement and snap
| Mechanic | Owner | Required behavior |
|---|---|---|
| Snap-point provider | PlacementSnap | Generate candidate points for a piece-specific grid/socket rule. |
| Candidate filters/ranking | PlacementSnap | Filter by type, normal, axis, distance, depth, revision and capabilities before deterministic scoring. |
| Surface anchor | PlacementSnap | Stable anchor ID plus the source surface/revision. |
| Ghost preview | PlacementSnap | Show candidate transform and clear invalidity reasons. |
| Full-volume overlap | PlacementSnap | Validate the entire collision footprint, not just pivot position. |
| Commit revalidation | PlacementSnap | Recheck collision, target capability and source revision at confirmation time. |
| Stale-anchor invalidation | PlacementSnap | Excavation changes invalidate dependent snap candidates. |
| Configurable plank grid | PlacementSnap | Support 0.20 m plank spacing without hard-coding it into scanner logic. |
| Placement transaction | PlacementSnap/Shoring | Failed spawn/link/inventory debit is compensated. |

## E. Excavation and shoring
| Mechanic | Owner | Required behavior |
|---|---|---|
| Dig authorization | Excavation | Require a current observation, EditableVoxel capability and eligible tool/target. |
| Fill operation | Excavation | Same validation, revision, transaction and rollback policy as dig. |
| Operation idempotency | Excavation | Duplicate operation IDs are not applied twice. |
| Dirty-region event | Excavation | Publish changed cells/chunks and new revisions. |
| Shape/depth limits | Excavation | Bound radius/depth/volume by profile and operation type. |
| Trench layout planner | Shoring | Create stations along a bounded axis/width/depth. |
| Station spacing | Shoring | Configurable center-to-center spacing; 1.5 m is only a prototype reference. |
| Upright/post | Shoring | Separate physical piece with anchors, health/durability and state. |
| Lagging/waling board | Shoring | Separate wall-facing piece linked to uprights. |
| Cross strut | Shoring | Link opposing supports; validate span and anchors. |
| Base/seat/platform/ladder | Shoring | Optional families with independent placement contracts. |
| Preview vs commit | Shoring | Planner proposes; explicit commit installs physical pieces. |
| Missing-part report | Shoring | Identify incomplete coverage, invalid links and conflicts. |
| Terrain-change invalidation | Shoring | Revalidate affected plans/anchors after density changes. |
| Carry/remove/repair/replace | Shoring | Explicit lifecycle and resource conservation on failure. |

## F. Structural support, damage and collapse
| Mechanic | Owner | Required behavior |
|---|---|---|
| Support graph | StructuralIntegrity | Nodes are pieces; edges are explicit, validated anchor/socket/contact relations. |
| Capacity/load model | StructuralIntegrity | Deterministic configurable gameplay model, not real geotechnical engineering. |
| Invalid graph/cycle handling | StructuralIntegrity | Detect duplicate IDs, dangling links and unsupported cycles. |
| Damage/durability | StructuralIntegrity | Health changes capacity or disables connections according to declared rules. |
| Repair/replace | StructuralIntegrity + Shoring | Update state/graph atomically. |
| Support-loss propagation | StructuralIntegrity | Reassess the affected subgraph after removal/failure. |
| Collapse decision | StructuralIntegrity | Emit explicit partial/full collapse events; physics adapter enacts them. |
| Collapse latch | StructuralIntegrity + Persistence | Reload cannot silently revive a failed piece. |

## G. Survival and environmental data
| Mechanic | Owner | Required behavior |
|---|---|---|
| Health/stamina/hunger/thirst | Existing canonical SurvivalComponent through adapter | Do not create a second authoritative player-stat owner. |
| Operation effort | Survival | Separate costs for swings, heavy digging, shoring carry/place/repair and heavy carrying. |
| Temperature/wetness | Survival + WorldEnvironment | Update exposure from bounded, timestamped samples. |
| Underground/depth exposure | Survival | Track underground flag, depth, darkness and water contact. |
| Shelter/fire warmth | WorldEnvironment provider | Supply environmental input; do not duplicate fire state. |
| Water ingress/drainage | WorldEnvironment | Keep water/soil state separate from density; define explicit coupling. |
| Missing/stale samples | Survival | Unknown follows a policy; is not silently considered safe. |
| Exposure snapshot | Survival + Persistence | Save versioned accumulators/timestamps. |
| Access/escape | Shoring/PlacementSnap with later movement adapter | Ladder/platform/rope express access; movement checks belong to engine adapter. |
| Optional gas/oxygen | Future extension | Not required for the initial core loop. |

## H. Persistence and vertical slice
| Mechanic | Owner | Required behavior |
|---|---|---|
| Schema/map/profile identity | Persistence | Identify schema, world, runtime profile and grid. |
| Voxel snapshot | TerrainVoxel + Persistence | Persist authoritative sparse deltas, not generated meshes. |
| Shoring record | Persistence | Save IDs, type/class, transform, plan/anchors, material/health/state and collapse latch. |
| Stone records | Persistence | Support only declared loose/installed/falling states; reject unsupported carry/drag/fracture states. |
| Player/inventory/exposure | Persistence adapters | Validate through canonical owners before restore. |
| Preflight | Persistence | Validate all payloads and references before live mutation. |
| Staging/rollback | Persistence | Preserve old world if staging or commit fails. |
| Revalidation gate | Persistence + MeshCollision + StructuralIntegrity | Restored supports stay inactive until surfaces/collision are current and links revalidate. |
| Legacy migration | Persistence | Migrate known older schemas without inventing absent data. |
| Full vertical slice | Validation | Scan → classify → dig → collision → shoring → damage/support loss → save → reload → revalidate. |

## Related mechanics not duplicated here
Wood/log harvesting, generic inventory/crafting, material interaction M1–M6, fire/traps/raids, crop/animal simulation, global chunk events, water/soil/decay/chemistry and exploration journals already have code families elsewhere in the repository. They are integration peers, not duplicate state owners; see [RELATED_MECHANICS.md](RELATED_MECHANICS.md).
