# Responsible Disclosure Policy

DeVault takes security very seriously.  We greatly appreciate any and all disclosures of bugs and vulnerabilities that are done in a responsible manner.  We will engage responsible disclosures according to this policy and put forth our best effort to fix disclosed vulnerabilities as well as reaching out to node operators to deploy fixes in a timely manner.

## Responsible Disclosure Guidelines

Do not disclose any bug or vulnerability on public forums, message boards, mailing lists, etc. prior to responsibly disclosing to DeVault and giving sufficient time for the issue to be fixed and deployed.
Do not execute on or exploit any vulnerability.  This includes testnet, as both mainnet and testnet exploits are effectively public disclosure.  Regtest mode may be used to test bugs locally.

## Reporting a Bug or Vulnerability

When reporting a bug or vulnerability, please provide the following to security@devault.cc:
* A short summary of the potential impact of the issue (if known).
* Details explaining how to reproduce the issue or how an exploit may be formed.
* Your name (optional).  If provided, we will provide credit for disclosure.  Otherwise, you will be treated anonymously and your privacy will be respected.
* Your email or other means of contacting you.
* A PGP key/fingerprint for us to provide encrypted responses to your disclosure.  If this is not provided, we cannot guarantee that you will receive a response prior to a fix being made and deployed.

## Encrypting the Disclosure

We highly encourage all disclosures to be encrypted to prevent interception and exploitation by third-parties prior to a fix being developed and deployed.  Please encrypt using the PGP public key with fingerprint: `E624CA68344A4AB5DE16183C54094D41BC4E0E77`

It may be obtained via:
```
gpg --recv-keys E624CA68344A4AB5DE16183C54094D41BC4E0E77
```

Below are some basic instructions for encrypting your disclosure on Linux if you are unfamiliar with GPG:

1. If you don't already have GPG, install it:
For Debian/Ubuntu based distributions:
```
sudo apt-get install gpg
```
For Archlinux based distributions:
```
pacman -S gnupg
```
2. Save your disclosure report to a plain text file, then encrypt it:
```
gpg --armor --encrypt --recipient security@devault.cc mydisclosurefile
```
3. Email the resulting `.asc` file to security@devault.cc.

## Backup PGP Keys

This PGP fingerprint and email is provided only as a backup in case you are unable to contact DeVault via the security email above.

#### proteus
```
pro at proteanx dot com
E624CA68344A4AB5DE16183C54094D41BC4E0E77
```
