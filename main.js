const particle_count = 1000;

const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');

const step = Module.cwrap('step', null, ['number']);
const set_tool_status = Module.cwrap('set_tool_status', null, ['number', 'number', 'number']);

function get_physics_states() {
    const ptr = Module.ccall('get_physics_states', 'number');
    // pointer size is 4 bytes
    const ptr_x = Module.HEAP32[ptr>>2];
    const ptr_y = Module.HEAP32[(ptr+4*1)>>2];
    const ptr_x_accel = Module.HEAP32[(ptr+4*2)>>2];
    const ptr_y_accel = Module.HEAP32[(ptr+4*3)>>2];
    const physics_states = {
        x: new Float32Array(Module.HEAPF32.buffer,
                            ptr_x,
                            particle_count),
        y: new Float32Array(Module.HEAPF32.buffer,
                            ptr_y,
                            particle_count),
        x_accel: new Float32Array(Module.HEAPF32.buffer,
                            ptr_x_accel,
                            particle_count),
        y_accel: new Float32Array(Module.HEAPF32.buffer,
                            ptr_y_accel,
                            particle_count),
    };
    return physics_states;
}

function main() {
    window.addEventListener('resize', resize);
    resize();

    window.addEventListener('mousedown', mousedown);
    window.addEventListener('mousemove', mousemove);
    window.addEventListener('mouseup', mouseup);

    function resize() {
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;
    }

    let is_tool_active = false;
    function mousedown(event) {
        if (event.button === 0) {
            is_tool_active = true;
            set_tool_status(event.clientX, event.clientY, is_tool_active);
        }
    }
    function mousemove(event) {
        if (is_tool_active) {
            set_tool_status(event.clientX, event.clientY, is_tool_active);
        }
    }
    function mouseup(event) {
        is_tool_active = false;
        set_tool_status(0, 0, is_tool_active);
    }

    const text_start = 11.0;
    const text_start_fade_length = 1.0;
    const text_length = 2.0;
    const text_end_fade_length = 2.0;
    const text_end = text_start + text_start_fade_length + text_length + text_end_fade_length;
    let text_alpha = 0.0;
    let text = 'Click & drag to pull';

    Module.ccall('init', null, ['number', 'number', 'number'], [canvas.width, canvas.height, particle_count]);
    const physics_states = get_physics_states();

    const start_time = Date.now();
    let prev_time = start_time;
    let curr_time = prev_time;
    let start_delta = 0.0;
    let delta = start_delta;
    requestAnimationFrame(frame);
    function frame() {
        curr_time = Date.now();
        delta = (curr_time - prev_time) / 1000;
        start_delta = (curr_time - start_time) / 1000;
        prev_time = curr_time;
        
        if (text_start > start_delta || start_delta > text_end) {
            text_alpha = 0.0;
        }
        else if (text_start < start_delta && start_delta < text_start + text_start_fade_length) {
            text_alpha = (start_delta - text_start) / text_start_fade_length;
        }
        else if (start_delta < text_start + text_start_fade_length + text_length) {
            text_alpha = 1.0;
        }
        else if (start_delta < text_end) {
            text_alpha = 1.0 - (start_delta - text_start - text_start_fade_length - text_length) / text_end_fade_length;
        }

        step();

        render();
        
        requestAnimationFrame(frame);
    }

    function render() {
        ctx.fillStyle = 'black';
        ctx.fillRect(0, 0, canvas.width, canvas.height);

        if (text_alpha > 0.01) {
            ctx.font = '48px sans-serif';
            ctx.fillStyle = 'rgba(170, 170, 170, '+ text_alpha +')';
            const text_width = ctx.measureText(text).width;
            ctx.fillText(text, canvas.width / 2 - text_width / 2 - 20, 72);
        }

        for (let i = 0; i < particle_count; i += 1) {
            const x = physics_states.x[i];
            const y = physics_states.y[i];
            
            if (i % 3 === 0) {
                ctx.fillStyle = '#b00';
            }
            if (i % 3 === 1) {
                ctx.fillStyle = '#e70';
            }
            if (i % 3 === 2) {
                ctx.fillStyle = '#02e';
            }
            ctx.fillRect(x, y, 2, 2);
        }
    }
}

Module.postRun.push(main);