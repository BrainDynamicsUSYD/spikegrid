CREATE TABLE IF NOT EXISTS cson_session (
    id VARCHAR(50) PRIMARY KEY NOT NULL, -- session ID string
    last_saved INTEGER NOT NULL DEFAULT 0, -- Unix Epoch timestamp
    json TEXT DEFAULT NULL -- the raw session JSON
);
