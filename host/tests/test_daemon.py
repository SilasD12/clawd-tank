import asyncio
import pytest
from unittest.mock import AsyncMock
from clawd_tank_daemon.daemon import ClawdDaemon


@pytest.mark.asyncio
async def test_handle_add_tracks_notification():
    daemon = ClawdDaemon()
    msg = {"event": "add", "session_id": "s1", "project": "proj", "message": "hi"}
    await daemon._handle_message(msg)
    assert "s1" in daemon._active_notifications
    assert daemon._pending_queue.qsize() == 1


@pytest.mark.asyncio
async def test_handle_dismiss_removes_notification():
    daemon = ClawdDaemon()
    await daemon._handle_message(
        {"event": "add", "session_id": "s1", "project": "p", "message": "m"}
    )
    await daemon._handle_message({"event": "dismiss", "session_id": "s1"})
    assert "s1" not in daemon._active_notifications
    assert daemon._pending_queue.qsize() == 2


@pytest.mark.asyncio
async def test_dismiss_unknown_is_safe():
    daemon = ClawdDaemon()
    await daemon._handle_message({"event": "dismiss", "session_id": "nope"})
    assert daemon._pending_queue.qsize() == 1


# --- Edge cases ---

@pytest.mark.asyncio
async def test_duplicate_add_updates_not_duplicates():
    """Adding the same session_id twice must update the entry, not create two."""
    daemon = ClawdDaemon()
    await daemon._handle_message(
        {"event": "add", "session_id": "s1", "project": "p", "message": "first"}
    )
    await daemon._handle_message(
        {"event": "add", "session_id": "s1", "project": "p", "message": "updated"}
    )
    assert len(daemon._active_notifications) == 1
    assert daemon._active_notifications["s1"]["message"] == "updated"
    # Both adds go to the queue for BLE delivery
    assert daemon._pending_queue.qsize() == 2


@pytest.mark.asyncio
async def test_empty_session_id_add_and_dismiss():
    """Empty-string session_id must be tracked and dismissable."""
    daemon = ClawdDaemon()
    await daemon._handle_message(
        {"event": "add", "session_id": "", "project": "p", "message": "m"}
    )
    assert "" in daemon._active_notifications

    await daemon._handle_message({"event": "dismiss", "session_id": ""})
    assert "" not in daemon._active_notifications


@pytest.mark.asyncio
async def test_multiple_sessions_independent():
    """Multiple independent session IDs must not interfere with each other."""
    daemon = ClawdDaemon()
    for sid in ("s1", "s2", "s3"):
        await daemon._handle_message(
            {"event": "add", "session_id": sid, "project": "p", "message": "m"}
        )
    assert len(daemon._active_notifications) == 3

    await daemon._handle_message({"event": "dismiss", "session_id": "s2"})
    assert len(daemon._active_notifications) == 2
    assert "s1" in daemon._active_notifications
    assert "s2" not in daemon._active_notifications
    assert "s3" in daemon._active_notifications


@pytest.mark.asyncio
async def test_unknown_event_does_not_crash_sender():
    """An unknown event in the queue must be logged and skipped, not crash _ble_sender."""
    daemon = ClawdDaemon()
    await daemon._handle_message({"event": "bogus", "session_id": "x"})
    await daemon._handle_message({"event": "dismiss", "session_id": "x"})

    from clawd_tank_daemon.protocol import daemon_message_to_ble_payload
    with pytest.raises(ValueError):
        daemon_message_to_ble_payload({"event": "bogus"})

    assert daemon._pending_queue.qsize() == 2


@pytest.mark.asyncio
async def test_ble_sender_skips_unknown_event():
    """_ble_sender must skip unknown events and continue processing the queue."""
    daemon = ClawdDaemon()
    daemon._ble = AsyncMock()
    daemon._ble.ensure_connected = AsyncMock()
    daemon._ble.write_notification = AsyncMock(return_value=True)

    await daemon._pending_queue.put({"event": "bogus", "session_id": "x"})
    await daemon._pending_queue.put({"event": "dismiss", "session_id": "d1"})

    sender = asyncio.create_task(daemon._ble_sender())
    await asyncio.sleep(0.1)
    daemon._running = False
    sender.cancel()
    try:
        await sender
    except asyncio.CancelledError:
        pass

    assert daemon._ble.write_notification.call_count >= 1
