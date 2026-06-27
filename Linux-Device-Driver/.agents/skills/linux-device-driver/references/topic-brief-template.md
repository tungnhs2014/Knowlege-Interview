# Topic Brief Template

Use this audit template for `coverage/topic-briefs/NN-<topic>.md`.

Topic Briefs are for source trace, merge decisions, gaps, and review evidence.
Do not paste the audit sections into `knowledge/` or `interview/`.

# Topic Brief - <NN> - <Topic>

## Output Targets
- Knowledge: `knowledge/NN-<topic>.md`
- Interview: `interview/NN-<topic>.md`
- Example: `examples/NN-<topic>/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-chNN` | `docs/...` | read/mapped/covered/merged/gap | ... |
| `ldd2-chNN` | `docs/...` | read/mapped/covered/merged/gap | ... |
| `notion-chNN-partM` | `docs/...` | read/mapped/covered/merged/gap | ... |

## Source Files Read
List exact files read and the headings/sections that matter.

## Merged Source Notes
Record shared concepts, unique details, contradictions, and what was chosen for the learner-facing docs.

## Source Differences
Record source conflicts, stale APIs, terminology drift, and version-sensitive behavior.

## Gaps / Uncertainties
Record missing pieces, unclear source claims, or content that should be handled in another learning-path topic.

## External Validation
Record external sources used, preferably `docs.kernel.org`, kernel source documentation, or kernel.org release information.

## Learning Content Brief
Summarize the mental model, core mechanism, APIs, lifecycle, examples, common bugs, debugging notes, production concerns, and interview angles.
