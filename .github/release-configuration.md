# GitHub Release Configuration

The workflow in `.github/workflows/release-windows.yml` expects the following
repository-level GitHub Actions variables and secrets.

## Variables

| Name | Meaning | Example |
| --- | --- | --- |
| `ARTIFACT_SIGNING_ENDPOINT` | Azure Trusted Signing endpoint URI | `https://eus.codesigning.azure.net/` |
| `ARTIFACT_SIGNING_ACCOUNT_NAME` | Trusted Signing account name | `fieldmouse-signing` |
| `ARTIFACT_SIGNING_CERTIFICATE_PROFILE` | Trusted Signing certificate profile name | `fieldmouse-public` |

## Secrets

| Name | Meaning |
| --- | --- |
| `AZURE_CLIENT_ID` | Azure app registration client ID used by `azure/login` |
| `AZURE_TENANT_ID` | Azure tenant ID for Trusted Signing |
| `AZURE_SUBSCRIPTION_ID` | Azure subscription ID containing the signing resources |

`GITHUB_TOKEN` is provided automatically by GitHub Actions and does not need to
be created manually.

## Fast Path: GitHub CLI

Prerequisites:

1. Install GitHub CLI.
2. Run `gh auth login`.
3. Ensure you have admin access to the repository.

Then run:

```powershell
.\scripts\configure-github-release.ps1 \
  -Repository owner/field-mouse \
  -ArtifactSigningEndpoint https://eus.codesigning.azure.net/ \
  -ArtifactSigningAccountName your-signing-account \
  -ArtifactSigningCertificateProfile your-certificate-profile \
  -AzureClientId YOUR_AZURE_CLIENT_ID \
  -AzureTenantId YOUR_AZURE_TENANT_ID \
  -AzureSubscriptionId YOUR_AZURE_SUBSCRIPTION_ID
```

If you do not want secrets on the command line, omit the secret parameters and
use `-PromptForSecrets`.

## Manual Setup

Repository settings path:

1. `Settings`
2. `Secrets and variables`
3. `Actions`
4. Add the variables and secrets listed above

## Notes

* The Azure app used for signing needs permission to authenticate with the
  Trusted Signing account and certificate profile.
* The workflow signs every shipped `.exe` and `.dll` in the staged release bundle
  before creating the ZIP, then signs the generated installer EXE separately.