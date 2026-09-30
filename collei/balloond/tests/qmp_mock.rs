use std::time::Duration;

use collei_balloond::qmp::{QmpClient, QmpError};
use serde_json::{Value, json};
use tempfile::tempdir;
use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
use tokio::net::UnixListener;

#[tokio::test]
async fn matches_responses_and_ignores_events() {
    let directory = tempdir().unwrap();
    let socket = directory.path().join("qmp.sock");
    let listener = UnixListener::bind(&socket).unwrap();
    let server = tokio::spawn(async move {
        let (stream, _) = listener.accept().await.unwrap();
        let (reader, mut writer) = stream.into_split();
        let mut reader = BufReader::new(reader);
        writer
            .write_all(b"{\"QMP\":{\"version\":{},\"capabilities\":[]}}\r\n")
            .await
            .unwrap();

        let capability = read_request(&mut reader).await;
        writer
            .write_all(format!("{{\"return\":{{}},\"id\":{}}}\r\n", capability["id"]).as_bytes())
            .await
            .unwrap();

        let query = read_request(&mut reader).await;
        writer
            .write_all(b"{\"event\":\"RESET\",\"data\":{}}\r\n")
            .await
            .unwrap();
        let response = format!(
            "{{\"return\":{{\"status\":\"running\"}},\"id\":{}}}\r\n",
            query["id"]
        );
        let split = response.len() / 2;
        writer
            .write_all(&response.as_bytes()[..split])
            .await
            .unwrap();
        writer
            .write_all(&response.as_bytes()[split..])
            .await
            .unwrap();
    });

    let mut client = QmpClient::connect(&socket, Duration::from_secs(1))
        .await
        .unwrap();
    let response = client.execute("query-status", None).await.unwrap();
    assert_eq!(response, json!({"status": "running"}));
    server.await.unwrap();
}

#[tokio::test]
async fn reports_qmp_command_errors() {
    let directory = tempdir().unwrap();
    let socket = directory.path().join("qmp.sock");
    let listener = UnixListener::bind(&socket).unwrap();
    let server = tokio::spawn(async move {
        let (stream, _) = listener.accept().await.unwrap();
        let (reader, mut writer) = stream.into_split();
        let mut reader = BufReader::new(reader);
        writer
            .write_all(b"{\"QMP\":{\"version\":{},\"capabilities\":[]}}\r\n")
            .await
            .unwrap();
        let capability = read_request(&mut reader).await;
        writer
            .write_all(format!("{{\"return\":{{}},\"id\":{}}}\r\n", capability["id"]).as_bytes())
            .await
            .unwrap();
        let query = read_request(&mut reader).await;
        writer
            .write_all(
                format!(
                    "{{\"error\":{{\"class\":\"GenericError\",\"desc\":\"boom\"}},\"id\":{}}}\r\n",
                    query["id"]
                )
                .as_bytes(),
            )
            .await
            .unwrap();
    });

    let mut client = QmpClient::connect(&socket, Duration::from_secs(1))
        .await
        .unwrap();
    let error = client.execute("query-status", None).await.unwrap_err();
    assert!(matches!(error, QmpError::Command { .. }));
    assert!(error.to_string().contains("boom"));
    server.await.unwrap();
}

async fn read_request(reader: &mut BufReader<tokio::net::unix::OwnedReadHalf>) -> Value {
    let mut line = String::new();
    reader.read_line(&mut line).await.unwrap();
    serde_json::from_str(&line).unwrap()
}
