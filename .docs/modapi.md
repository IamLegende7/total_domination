# Modding API

## Exitcodes

### ModServer responses

| Code | Sender      | Description
|:----:|:------------|:--------------
| `-1` | `ModServer` | Requesting function
| `0`  | `ModServer` | OK
| `1`  | `ModServer` | Misc error
| `2`  | `ModServer` | Unknown request type
| `3`  | `ModServer` | ill formed request
| `4`  | `ModServer` | invalid mod source code

### Function request responses

| Code | Sender               | Description
|:----:|:---------------------|:--------------
| `0`  | `ModServerConnector` | OK
| `1`  | `ModServerConnector` | Misc error
| `2`  | `ModServerConnector` | Unknown function
