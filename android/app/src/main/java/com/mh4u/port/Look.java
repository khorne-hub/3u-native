package com.mh4u.port;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.Locale;

/**
 * The app's screens in the game's style, and the app's colours, all of them here (every screen takes them from Look):
 * the MH3U's water, as the app's icon (res/mipmap-.../ic_launcher: a deep sea-green plate, a gold ring with gold waves on
 * it) - sea-green pages and plates with a gold edge, gold titles, foam-white text, a coral warning. The native side's
 * app_palette.h has the same values. notice() is the game-styled notice the new screens use (the save folder, the
 * official-build and update notices): it never comes out wider than the screen, and its buttons stack when there are more
 * than two, so a long label is never cut.
 */
final class Look {
    /** A whole page, a notice (the icon's face; the MH4U's wine). */
    static final int PAGE = Color.rgb(14, 40, 50);
    /** A box on a page: the folder's tree, a log (darker, the icon's corners). */
    static final int PANEL = Color.rgb(5, 18, 24);
    /** Text (the MH4U's cream). */
    static final int TEXT = Color.rgb(232, 242, 240);
    /** Titles, the main button's label (the icon's gold). */
    static final int GOLD = Color.rgb(226, 182, 92);
    /** A button's face (the MH4U's bronze). */
    static final int BUTTON = Color.rgb(22, 70, 86);
    /** The plates' and buttons' edge, the rules under the titles (the icon's ring, a shade darker). */
    static final int EDGE = Color.rgb(200, 160, 80);
    /** A warning, a step that failed (coral: it reads as one on any colour). */
    static final int ALERT = Color.rgb(255, 150, 120);
    /** A button held, or picked with a pad or the keys (the sea-green the menu's picked row is lit with). */
    static final int BUTTON_LIT = Color.rgb(34, 108, 128);
    /** The picked button's edge (the menu's gold glow). */
    static final int GLOW = Color.rgb(255, 214, 110);
    /** A button's label while it does nothing (greyed). */
    static final int OFF = Color.rgb(98, 126, 132);

    private Look() {}

    static boolean es() { return "es".equals(Locale.getDefault().getLanguage()); }

    static int dp(Context c, int v) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, v, c.getResources().getDisplayMetrics());
    }

    static TextView text(Context c, CharSequence s, int sp, int color, boolean bold) {
        TextView t = new TextView(c);
        t.setText(s);
        t.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        t.setTextColor(color);
        if (bold) t.setTypeface(Typeface.DEFAULT_BOLD);
        return t;
    }

    static GradientDrawable plate(Context c, int fill, int radius) {
        return plate(c, fill, radius, EDGE, 2);
    }

    static GradientDrawable plate(Context c, int fill, int radius, int edge, int width) {
        GradientDrawable d = new GradientDrawable();
        d.setColor(fill);
        d.setStroke(dp(c, width), edge);
        d.setCornerRadius(dp(c, radius));
        return d;
    }

    /**
     * A button's face in every state, as the menu's: a sea-green plate with the gold edge; held, lit sea-green; picked with
     * a pad or the keys (focused), lit with a gold glow round it and a white label (instead of the system's own focus
     * mark, which was a lime frame on some handhelds); greyed while it does nothing. Call it after the label's colour is
     * set.
     */
    static void dress(Button b, int radius) {
        Context c = b.getContext();
        StateListDrawable s = new StateListDrawable();
        s.addState(new int[] {-android.R.attr.state_enabled}, plate(c, BUTTON, radius, Color.argb(110, 200, 160, 80), 2));
        s.addState(new int[] {android.R.attr.state_pressed}, plate(c, BUTTON_LIT, radius, GLOW, 2));
        s.addState(new int[] {android.R.attr.state_focused}, plate(c, BUTTON_LIT, radius, GLOW, 3));
        s.addState(new int[] {}, plate(c, BUTTON, radius));
        b.setBackground(s);
        b.setDefaultFocusHighlightEnabled(false);
        // (the label: white on the lit plate, as the menu's picked row; greyed while off)
        b.setTextColor(new ColorStateList(new int[][] {{-android.R.attr.state_enabled}, {android.R.attr.state_pressed},
                {android.R.attr.state_focused}, {}}, new int[] {OFF, TEXT, TEXT, b.getCurrentTextColor()}));
    }

    static Button button(Context c, String s, boolean main) {
        Button b = new Button(c);
        b.setText(s);
        b.setAllCaps(false);
        b.setTextColor(main ? GOLD : TEXT);
        b.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        if (main) b.setTypeface(Typeface.DEFAULT_BOLD);
        dress(b, 6);
        b.setPadding(dp(c, 18), 0, dp(c, 18), 0);
        return b;
    }

    /** One button of a notice: its label, whether it is the main one (gold), and what it does (the notice closes first). */
    static final class Choice {
        final String label;
        final boolean main;
        final Runnable action;
        Choice(String label, boolean main, Runnable action) { this.label = label; this.main = main; this.action = action; }
    }

    /**
     * A notice over `a`: a gold title, the text (it scrolls on a short screen), then the choices. The system's Back
     * presses the last one; a touch outside does nothing. Shown at once; the dialog is returned.
     */
    static Dialog notice(Activity a, String title, CharSequence body, Choice... choices) {
        Dialog d = new Dialog(a);
        d.requestWindowFeature(Window.FEATURE_NO_TITLE);
        d.setCancelable(false);
        LinearLayout box = new LinearLayout(a);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setBackground(plate(a, PAGE, 8));
        box.setPadding(dp(a, 22), dp(a, 14), dp(a, 22), dp(a, 16));
        TextView head = text(a, title, 18, GOLD, true);
        head.setGravity(Gravity.CENTER);
        box.addView(head);
        View rule = new View(a);
        rule.setBackgroundColor(EDGE);
        LinearLayout.LayoutParams ruleSize = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(a, 1));
        ruleSize.setMargins(0, dp(a, 8), 0, dp(a, 10));
        box.addView(rule, ruleSize);
        TextView t = text(a, body, 15, TEXT, false);
        t.setLineSpacing(dp(a, 2), 1);
        ScrollView scroll = new ScrollView(a);
        scroll.addView(t);
        box.addView(scroll, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, 0, 1));
        LinearLayout row = new LinearLayout(a);
        boolean stacked = choices.length > 2;
        row.setOrientation(stacked ? LinearLayout.VERTICAL : LinearLayout.HORIZONTAL);
        row.setPadding(0, dp(a, 12), 0, 0);
        Button first = null;
        for (Choice ch : choices) {
            Button b = button(a, ch.label, ch.main);
            b.setOnClickListener(v -> {
                d.dismiss();
                if (ch.action != null) ch.action.run();
            });
            LinearLayout.LayoutParams lp = stacked
                    ? new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, dp(a, 44))
                    : new LinearLayout.LayoutParams(0, dp(a, 46), 1);
            lp.setMargins(stacked ? 0 : dp(a, 5), dp(a, stacked ? 3 : 0), stacked ? 0 : dp(a, 5), dp(a, stacked ? 3 : 0));
            row.addView(b, lp);
            if (first == null) first = b;
        }
        box.addView(row);
        final Choice last = choices.length > 0 ? choices[choices.length - 1] : null;
        d.setOnKeyListener((dlg, key, event) -> {  // (Back, or a pad's B: the last choice, "Cancel" / "Later" / "Exit")
            if (key != KeyEvent.KEYCODE_BACK && key != KeyEvent.KEYCODE_BUTTON_B && key != KeyEvent.KEYCODE_ESCAPE) return false;
            if (event.getAction() == KeyEvent.ACTION_UP && last != null) {
                d.dismiss();
                if (last.action != null) last.action.run();
            }
            return true;
        });
        d.setContentView(box);
        Window w = d.getWindow();
        if (w != null) {
            w.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
            DisplayMetrics m = a.getResources().getDisplayMetrics();
            w.setLayout(Math.min(dp(a, 600), (int) (m.widthPixels * .92f)), android.view.WindowManager.LayoutParams.WRAP_CONTENT);
        }
        // (a box taller than the screen: the window's height is at most the screen's, and the text shrinks and scrolls
        // while the buttons stay: the weighted ScrollView takes what is left)
        d.show();
        if (first != null) first.requestFocus();  // (a pad: A presses the first)
        return d;
    }
}
