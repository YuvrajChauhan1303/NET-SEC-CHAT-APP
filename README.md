# TODO

## Phase 1

- [x] multi-user login support via `fork()`.
- [x] maintaining logs (sort of. not in file.)
- [x] implement `quit` functionality.
- [x] user quit handling (user may leave without order).
- [x] display online users `\who`
- [x] message forwarding
- [x] chat selection `@username <msg>`


## Phase 2

- [x] implement Diffie-Hellman key exchange manually using modular exponentiation.
- [x] use a standard published prime group (RFC 3526 etc.).
- [x] separate DH exchange for every client-server connection.
- [ ] derive AES key from DH shared secret using a hash.
- [ ] understand + mention in report why we hash the DH secret.
- [ ] encrypt all client-server communication using AES-GCM.
- [ ] encrypt login/registration messages too.
- [ ] print shared-secret fingerprint on both client + server.
- [ ] verify both sides get the same fingerprint.
- [ ] repeat Wireshark capture and verify messages are now ciphertext.
- [ ] test AES-GCM tampering detection by modifying 1 byte.
- [ ] verify modified ciphertext gets rejected.
- [ ] build separate MITM proxy (Mallory).
- [ ] make Mallory perform separate DH exchange with client + server.
- [ ] make Mallory decrypt/read/forward messages.
- [ ] log plaintext messages at Mallory.
- [ ] demonstrate client + server think they are talking directly.
- [ ] explain in report why DH alone doesn't prevent MITM.
- [ ] explain what evidence could expose the MITM.
- [ ] run Mallory on separate VM between client + server.
- [ ] take screenshots/logs for DH fingerprints.
- [ ] take Wireshark screenshots (Phase 1 plaintext vs Phase 2 ciphertext).
- [ ] take screenshots/logs for tamper detection.
- [ ] take screenshots/logs for successful MITM attack.
- [ ] clean up + submit DH, client, server + MITM source code.