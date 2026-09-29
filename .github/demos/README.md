# Screenshots

This directory holds the screenshots shown in the "Preview" section of
[README.md](../README.md) and [README-RU.md](../README-RU.md).

## Expected files

| File | Used for |
|---|---|
| `history1.png` | Chat history view with the chain top bar and floating date header |
| `history2.png` | Chat view |
| `settings.png` | Settings window |

Both READMEs reference these three paths, so the file names and the `.png`
extension must stay as listed.

## How to add or replace them

1. Capture a screenshot of the running client (x86_64 Linux build).
2. Name it as in the table above and place it in this directory.
3. Commit and push:

```bash
git add .github/demos/
git commit -m "docs: update screenshots"
git push main
```

Images are rendered by GitHub relative to the repository root, so a width of
about `268` px works well for the three-column table. PNG is preferred to keep
the repository size stable.
