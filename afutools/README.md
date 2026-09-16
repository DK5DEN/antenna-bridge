# afu.tools: rig profile catalog

Server side of the profile sharing for antenna-bridge, deployed into the
afu.tools stack (FastAPI + nginx + static site).

| Path | Purpose |
| --- | --- |
| `api/rigprofile.py` | SQLite store and validation (same rules as the firmware), public/own visibility, admin functions |
| `api/rigprofile_admin.py` | administration page `/admin/rigs` built with the helpers of `adminseite.py` |
| `api/app_rig_patch.py` | idempotent patch: imports, init, routes, open GET for the bridge, admin navigation entry |
| `site/antenna-bridge.html`, `site/js/antenna-bridge.js` | catalog page: list, detail, submission form, own submissions, prefill from the bridge (`#teilen=`) |
| `tools/patch-navigation.py` | menu entry in `builder/navigation.py` |
| `tools/deploy.sh` | copies everything, patches, rebuilds the api container, regenerates the navigation |

API (`/api/v1/antenna-bridge/rigs`): `GET` list (released, plus own with
`?melder=`), `GET /{id}` document, `POST` `{melder, rufzeichen, profile}`,
`DELETE /{id}?melder=`. GET is open without an API key so the bridge's own
page can read the catalog from any LAN address. Submissions start as `neu`
and are released or rejected under `/admin/rigs`.
