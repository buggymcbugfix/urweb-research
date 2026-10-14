// What moves on its own, for urt's browser runs: a panel that slides in,
// a pulse of two iterations, a fade, a spinner that never stops, smooth
// scrolls, a box to scroll with the wheel, and the tab's visibility.
// Every line of the log says when, in ms of the page's own clock.
function motionSetup() {
    if (document.getElementById('log')) { return; }
    const style = document.createElement('style');
    style.textContent = `
      #panel { position: absolute; left: -300px; top: 40px; width: 200px; transition: left 600ms ease-out 100ms; }
      #panel.in { left: 20px; }
      @keyframes pulse { from { opacity: 1 } to { opacity: 0.3 } }
      #pulse.go { animation: pulse 250ms linear 2; }
      @keyframes spin { to { transform: rotate(360deg) } }
      #spinner { display: inline-block; animation: spin 1s linear infinite; }
      #bar { position: fixed; top: 0; right: 0; }
      #box { position: fixed; top: 40px; right: 0; width: 120px; height: 100px; overflow: auto; }
      #inner { height: 1000px; }
      #tall { height: 3000px; }
      #mark { position: absolute; top: 1200px; }
      html { scroll-behavior: smooth; }`;
    document.head.appendChild(style);
    const add = (parent, tag, attrs, text) => {
        const e = document.createElement(tag);
        for (const k in attrs) { e.setAttribute(k, attrs[k]); }
        if (text) { e.textContent = text; }
        parent.appendChild(e);
        return e;
    };
    const t = () => Math.round(performance.now());
    const root = add(document.body, 'div', {id: 'top'});
    const log = add(root, 'pre', {id: 'log'});
    const say = m => { log.textContent += t() + ' ' + m + '\n'; };
    const bar = add(root, 'div', {id: 'bar'});
    const button = (label, f) => { const b = add(bar, 'button', {}, label); b.addEventListener('click', f); return b; };
    add(root, 'span', {id: 'spinner'}, '*');

    const panel = add(root, 'div', {id: 'panel'});
    const inside = add(panel, 'button', {}, 'Inside');
    inside.addEventListener('click', () => say('inside at ' + Math.round(panel.getBoundingClientRect().left)));
    for (const e of ['transitionrun', 'transitionstart', 'transitionend']) { panel.addEventListener(e, () => say(e)); }
    button('Slide', () => { say('slide'); panel.classList.add('in'); });

    const pulse = add(root, 'p', {id: 'pulse'}, 'heart');
    for (const e of ['animationstart', 'animationiteration', 'animationend']) { pulse.addEventListener(e, () => say(e)); }
    button('Pulse', () => { say('pulse'); pulse.classList.add('go'); });

    const fade = add(root, 'p', {id: 'fade'}, 'ghost');
    button('Fade', () => {
        say('fade');
        fade.animate([{opacity: 1}, {opacity: 0}], {duration: 400, fill: 'forwards'}).finished.then(() => say('faded'));
    });

    // Whether a smooth scroll is under way half way through its time
    // (where exactly it is then is a matter of the engine's rounding).
    const half = to => {
        const from = window.scrollY;
        setTimeout(() => { const y = window.scrollY; say(y > Math.min(from, to) && y < Math.max(from, to) ? 'under way' : 'at ' + y); }, 150);
    };
    button('Down', () => { say('down'); half(1500); window.scrollTo({top: 1500, behavior: 'smooth'}); });
    button('Mark', () => { say('mark'); half(1200); document.getElementById('mark').scrollIntoView({behavior: 'smooth'}); });
    button('Jump', () => { say('jump'); window.scrollTo(0, 700); });
    const box = add(bar, 'div', {id: 'box'});
    add(box, 'div', {id: 'inner'}, 'Box');
    let wheels = 0;
    window.addEventListener('wheel', e => { wheels++; say('wheel ' + e.deltaY); });
    button('Where', () => say('at ' + Math.round(window.scrollY) + ', box at ' + box.scrollTop));
    add(root, 'div', {id: 'tall'});
    add(root, 'div', {id: 'mark'}, 'flag');

    let frames = 0;
    const count = () => { frames++; if (frames < 1000) { requestAnimationFrame(count); } };
    button('Count', () => { say('count'); frames = 0; requestAnimationFrame(count); });
    button('Frames', () => say(frames + ' frames, ' + document.visibilityState
                               + (document.hasFocus() ? ', focused' : '')));
    document.addEventListener('visibilitychange', () => say(document.visibilityState));
    window.addEventListener('focus', () => say('focus'));
    window.addEventListener('blur', () => say('blur'));
    button('Later', () => { say('later'); setTimeout(() => { say('alert'); alert('later'); say('alerted'); }, 1000); });
}
