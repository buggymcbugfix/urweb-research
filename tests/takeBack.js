// Two requests the page takes back in the task that sends them: an
// XMLHttpRequest aborted right after send(), and a fetch whose signal is
// aborted right after the call.
function takeBackSetup() {
    if (document.getElementById('xhr')) { return; }
    const button = (id, f) => {
        const b = document.createElement('button');
        b.id = id;
        b.textContent = id;
        b.onclick = f;
        document.body.appendChild(b);
    };
    button('xhr', () => {
        const x = new XMLHttpRequest();
        x.open('GET', '/note');
        x.send();
        x.abort();
    });
    button('fetch', () => {
        const c = new AbortController();
        fetch('/note', {signal: c.signal}).catch(() => {});
        c.abort();
    });
}
