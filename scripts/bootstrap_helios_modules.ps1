# Launch the Helios module tool. Requires Python 3.11+.
& python (Join-Path $PSScriptRoot "helios_modules.py") @args
exit $LASTEXITCODE
