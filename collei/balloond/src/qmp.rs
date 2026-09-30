use std::path::{Path, PathBuf};
use std::time::Duration;

use serde_json::{Value, json};
use thiserror::Error;
use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader, ReadHalf, WriteHalf};
use tokio::net::UnixStream;
use tokio::time::timeout;

#[derive(Debug, Error)]
pub enum QmpError {
    #[error("QMP I/O error via {path}: {source}")]
    Io {
        path: PathBuf,
        #[source]
        source: std::io::Error,
    },
    #[error("QMP operation timed out via {0}")]
    Timeout(PathBuf),
    #[error("QMP socket closed: {0}")]
    Closed(PathBuf),
    #[error("invalid QMP JSON via {path}: {source}")]
    Json {
        path: PathBuf,
        #[source]
        source: serde_json::Error,
    },
    #[error("invalid QMP greeting via {0}")]
    Greeting(PathBuf),
    #[error("QMP command {command} failed via {path}: {description}")]
    Command {
        path: PathBuf,
        command: String,
        description: String,
    },
    #[error("invalid QMP response to {command} via {path}")]
    Response { path: PathBuf, command: String },
}

pub struct QmpClient {
    path: PathBuf,
    reader: BufReader<ReadHalf<UnixStream>>,
    writer: WriteHalf<UnixStream>,
    operation_timeout: Duration,
    next_id: u64,
}

impl QmpClient {
    /// Connects to a QMP Unix socket and negotiates capabilities.
    ///
    /// # Errors
    ///
    /// Returns an error for connection failures, timeouts, invalid greetings,
    /// or a rejected capabilities command.
    pub async fn connect(path: &Path, operation_timeout: Duration) -> Result<Self, QmpError> {
        let path_buf = path.to_path_buf();
        let stream = timeout(operation_timeout, UnixStream::connect(path))
            .await
            .map_err(|_| QmpError::Timeout(path_buf.clone()))?
            .map_err(|source| QmpError::Io {
                path: path_buf.clone(),
                source,
            })?;
        let (reader, writer) = tokio::io::split(stream);
        let mut client = Self {
            path: path_buf,
            reader: BufReader::new(reader),
            writer,
            operation_timeout,
            next_id: 1,
        };
        let greeting = client.read_message().await?;
        if greeting.get("QMP").is_none() {
            return Err(QmpError::Greeting(client.path));
        }
        client.execute("qmp_capabilities", None).await?;
        Ok(client)
    }

    /// Executes one QMP command and returns its `return` value.
    ///
    /// # Errors
    ///
    /// Returns an error for I/O failures, timeouts, malformed responses, or a
    /// QMP error response. Asynchronous events are ignored while waiting.
    pub async fn execute(
        &mut self,
        command: &str,
        arguments: Option<Value>,
    ) -> Result<Value, QmpError> {
        let id = self.next_id;
        self.next_id += 1;
        let mut request = json!({"execute": command, "id": id});
        if let Some(arguments) = arguments {
            request["arguments"] = arguments;
        }
        let mut encoded = serde_json::to_vec(&request).map_err(|source| QmpError::Json {
            path: self.path.clone(),
            source,
        })?;
        encoded.push(b'\n');
        timeout(self.operation_timeout, self.writer.write_all(&encoded))
            .await
            .map_err(|_| QmpError::Timeout(self.path.clone()))?
            .map_err(|source| QmpError::Io {
                path: self.path.clone(),
                source,
            })?;

        loop {
            let response = self.read_message().await?;
            if response.get("event").is_some() {
                continue;
            }
            if response.get("id").and_then(Value::as_u64) != Some(id) {
                continue;
            }
            if let Some(error) = response.get("error") {
                let description = error
                    .get("desc")
                    .and_then(Value::as_str)
                    .unwrap_or("unknown QMP error")
                    .to_owned();
                return Err(QmpError::Command {
                    path: self.path.clone(),
                    command: command.to_owned(),
                    description,
                });
            }
            return response
                .get("return")
                .cloned()
                .ok_or_else(|| QmpError::Response {
                    path: self.path.clone(),
                    command: command.to_owned(),
                });
        }
    }

    async fn read_message(&mut self) -> Result<Value, QmpError> {
        let mut line = String::new();
        let bytes = timeout(self.operation_timeout, self.reader.read_line(&mut line))
            .await
            .map_err(|_| QmpError::Timeout(self.path.clone()))?
            .map_err(|source| QmpError::Io {
                path: self.path.clone(),
                source,
            })?;
        if bytes == 0 {
            return Err(QmpError::Closed(self.path.clone()));
        }
        serde_json::from_str(&line).map_err(|source| QmpError::Json {
            path: self.path.clone(),
            source,
        })
    }
}
